# Memory Management Subsystem

This document describes the memory management stack: early boot page tables, the
Physical Memory Manager (PMM), the paging backend (virtual memory manager), and the
SLUB allocator that sits on top.

All shared types, constants, and flags live in `mm_defs.hpp` (`namespace mem`) and
are used throughout `boot.asm`'s handoff, `pmm.cpp`, `paging.cpp`, and `slub.cpp`.

The stack initializes in this order:

1. **`boot.asm`** — 32-bit real-mode-to-long-mode transition; builds a minimal
   identity/HHDM/kernel page table set so the kernel can run in 64-bit mode.
2. **`PMM::initialize_bump`** — bump allocator, used before a full physical memory
   map exists.
3. **`PagingBackend::initialize`** — takes over the bootstrap page tables, maps all
   physical RAM into the Higher-Half Direct Map (HHDM).
4. **`PMM::initialize_buddy`** — builds the buddy allocator bitmaps and frees all
   usable RAM into it, retiring the bump allocator.
5. **`mem::initialize_slub`** — sets up `kmalloc` caches on top of the buddy
   allocator for general-purpose kernel heap allocation.

---

## 1. Boot-Time Paging (`boot.asm`)

Before the kernel proper runs, the bootloader stub builds a temporary set of page
tables (`p4_table`, `p3_table`, `p2_table`, each a static 4 KiB, page-aligned BSS
region) and enables long mode.

### Address spaces mapped

Three PML4 entries point at the same PDPT (`p3_table`), giving the first 1 GiB of
physical memory three simultaneous aliases:

| PML4 index | Virtual base | Purpose |
|---|---|---|
| 0 | `0x0000000000000000` | Identity map |
| 256 | `0xFFFF800000000000` | HHDM (Higher-Half Direct Map) base |
| 511 | `0xFFFFFFFF80000000` | Kernel map |

The single PDPT maps its first entry to `p2_table`, which is populated with 512
2 MiB huge pages (`Present | Writable | Huge`), covering the first 1 GiB of physical
RAM identically under all three aliases.


This bootstrap map is intentionally minimal (1 GiB, huge pages only) — it exists
only to get the kernel into a canonical higher-half virtual address space.
`PagingBackend::initialize` later tears down the identity mapping and builds the
real, fine-grained map.

---

## 2. Physical Memory Manager (`pmm.cpp` / `PMM`)

The PMM has two allocation backends that share the same public API
(`PMM::alloc_pages` / `PMM::free_pages`): a **bump allocator** used only until the
buddy allocator is ready, and a **buddy allocator** used for the remainder of
kernel life. `PMM::initialized_buddy` is the switch between the two.

### 2.1 Bump Allocator

Simple monotonic allocator used to hand out physical pages before a real memory
map / bitmap can be constructed (e.g. for the buddy allocator's own bookkeeping
structures).

State:
- `bump_ptr_phys` — next free physical address in the current region.
- `bump_region_end_phys` — end of the current usable region.
- `highest_reserved_phys` — highest physical address reserved by the kernel image
  or Multiboot info structures; nothing below this is handed out.

**`initialize_bump(mmap, multiboot_ptr)`**
Stores the Multiboot memory map pointer and computes `highest_reserved_phys` as
the higher of `kernel_end_phys` and the end of the Multiboot info blob.

**`find_bump_alloc_region(required_size)`**
Scans the Multiboot mmap for the next `MULTIBOOT_MEMORY_AVAILABLE` entry that:
- starts at or after the last-used region (`bump_region_end_phys`),
- isn't the zero page ((void*)0x0 == nullptr),
- can supply `required_size` bytes after skipping any overlap with
  `highest_reserved_phys`,
- stays entirely below `EARLY_BOOT_MAP_LIMIT` (bump allocations must live in
  memory that's already mapped — before the HHDM covers all of RAM).

Panics (`kernel_panic`) if no such region exists.

**`alloc_pages_bump(num)`**
Returns `nullptr` if the buddy allocator has already taken over. Otherwise
advances `bump_ptr_phys` by `num * PAGE_SIZE`, calling
`find_bump_alloc_region` as needed when the current region is exhausted.

> Note: the bump allocator has no `free`. Its allocations (buddy bitmaps, early
> page tables) are expected to live for the kernel's lifetime, or to be reclaimed
> in bulk once the buddy allocator frees "everything not reserved."

### 2.2 Buddy Allocator

Classic power-of-two buddy allocator with orders `0..BUDDY_MAX_ORDER`, tracked with
one **XOR bitmap per order** (`buddy_map[order]`) instead of per-block state:

- `list_add` / `list_remove` maintain a doubly linked free list per order
  (`free_lists[order]`), threaded through a `buddy_free_node` embedded in each
  free page (accessed via its HHDM virtual address).
- `toggle_buddy_bit(pfn, order)` flips the bit belonging to a buddy *pair* at a
  given order and returns the new value: `0` means both buddies are in the same
  state (both free or both used), `1` means they differ. This lets `free_pages`
  decide in O(1) whether it's safe to merge with the buddy, without needing a
  separate "is this page free" bit per page.

**`initialize_buddy(mmap)`**
1. Walks the mmap to compute `total_memory` and the highest usable physical
   address, validating that every entry's `type` is in the expected
   `[MULTIBOOT_MEMORY_AVAILABLE, MULTIBOOT_MEMORY_BADRAM]` range (panics on
   corruption).
2. Computes `total_pages` needed to cover that range and, for every order,
   the number of buddy-pairs (`max_pfn >> (order+1)`) and thus bitmap bytes
   required.
3. Bump-allocates enough pages to hold all order bitmaps contiguously, zeroes
   them, and slices `buddy_map[i]` pointers out of that block.
4. Walks the mmap a second time and calls `free_pages` on every 4 KiB page in
   every available region **except**: page 0 (NULL guard), the kernel image
   (`kernel_start_phys` .. `align_up(highest_reserved_phys)`), and everything
   between `highest_reserved_phys` and the final `bump_ptr_phys` (i.e. whatever
   the bump allocator itself consumed, including the buddy bitmaps).
5. Sets `initialized_buddy = true`. From this point `alloc_pages_bump` is
   permanently disabled.

**`alloc_pages(num)`**
- Delegates to `alloc_pages_bump` if the buddy allocator isn't up yet.
- Otherwise converts `num` to an order (`pages_to_order`, i.e.
  `ceil(log2(num))`, minimum order 0).
- Walks up from that order to find the smallest non-empty free list, pops the
  head block, and **splits** it back down to the requested order — at each split
  step the freed-off buddy half is pushed onto the lower order's free list and
  the corresponding bit is toggled.
- Updates `used_memory` / `free_memory` and returns the block's physical address
  (`pfn << PAGE_SHIFT`).
- Returns `nullptr` if the request exceeds `BUDDY_MAX_ORDER` or no memory is
  available.

**`free_pages(ptr, num)`**
- Converts `ptr` to a PFN and `num` to an order.
- Repeatedly toggles the buddy bit at the current order:
  - If the bit becomes `1`, the buddy is in use — stop merging, insert the
    block at the current order.
  - If it becomes `0`, both buddies are free — remove the buddy from its free
    list, merge (PFN becomes the aligned base of the pair), and continue at
    `order + 1`.
- Inserts the final (possibly merged) block into its free list and updates
  `used_memory` / `free_memory` by the *original* requested size.

**`is_page_free(pfn)` / `allocate_specific_page(pfn)`**
Linear scan over every order's free list to find the block containing `pfn`.
`allocate_specific_page` is used to carve a specific, already-known-physical
page (e.g. an MMIO region or a page reported used by firmware) out of the buddy
allocator: it removes the containing block, then repeatedly splits it down to
order 0, at each step keeping the half that contains the target PFN and freeing
the other half back into its own list — until the exact page is isolated and
marked used.

**`mark_region_free` / `mark_region_used`**
Convenience wrappers that round a `[base, base+length)` byte range to page
boundaries and call `free_pages` / `allocate_specific_page` per page — used to
reconcile the buddy allocator with regions discovered after initialization
(e.g. ACPI reclaimable memory, or hardware-reserved regions).

### 2.3 Statistics

`get_total_memory()`, `get_free_memory()`, `get_used_memory()` expose the running
counters maintained by the allocation/free paths.

---

## 3. Paging Backend (`paging.cpp` / `PagingBackend`)

The long-term x86_64 4-level (PML4 → PDPT → PD → PT) paging implementation, used
after `boot.asm`'s temporary tables are retired.

### CR3 / TLB primitives
`read_cr3`, `write_cr3`, `invlpg`, `flush_tlb_page` (single page invalidate via
`invlpg`), `flush_tlb_full` (reload `CR3` to flush all non-global entries).

### `initialize(mmap)`
1. Enables the NX bit in `EFER` if the CPU supports it.
2. Adopts the currently-loaded `CR3` (set up by `boot.asm`) as
   `kernel_pml4_phys`.
3. Clears PML4 entry 0 — **removes the identity map** installed at boot, leaving
   only the HHDM and kernel higher-half aliases — and flushes the TLB.
4. Re-scans the mmap for the highest usable physical address.
5. Maps `[EARLY_BOOT_MAP_LIMIT, end_phys)` into the HHDM
   (`phys + HHDM_BASE`) with `PageFlags::KernelRW`, one 4 KiB page at a time via
   `map_page`. (Memory below `EARLY_BOOT_MAP_LIMIT` is already covered by the
   boot-time 2 MiB huge-page mapping.)
6. Flushes the TLB again.

After this call, the entirety of usable physical RAM is addressable through the
HHDM, which is the assumption `PMM` and `KmemCache` rely on when converting
between physical addresses and pointers (`phys + HHDM_BASE`).

### Table walking

`get_or_create_subtable(entry, user)` is the shared "descend or allocate" helper:
if the entry is present it returns the existing child table (via
`phys_to_virt`); otherwise it calls `alloc_page_table()` (a zeroed page from the
PMM) and installs a new table entry.

### `map_page(virt, phys, flags)`
Walks PML4 → PDPT → PD → PT, creating intermediate tables on demand.
Supports:
- **1 GiB huge pages** (`PageFlags::Huge1G`) at the PDPT level — requires
  `CPU.has_pdpe1gb`; fails with `HardwareFault` otherwise, or
  `AlreadyMapped` if the PDPT entry is already present.
- **2 MiB huge pages** (`PageFlags::Huge2M`) at the PD level, same
  already-mapped semantics.
- **4 KiB pages** at the PT (leaf) level.

At each level, descending into a table that turns out to be a huge-page leaf
returns `AlreadyMapped` rather than corrupting the mapping. If the CPU lacks NX
support, the `no_execute` bit is forced off on the entry actually written (since
setting it without CPU support is undefined/reserved).

### `unmap_page(virt)`
Walks the same hierarchy without creating tables; returns `NotMapped` as soon as
it hits a non-present entry. Correctly detects and clears 1 GiB / 2 MiB huge
leaves as well as 4 KiB PTEs, then invalidates the TLB entry for `virt`.

Note: this does **not** free now-empty intermediate page tables back to the PMM —
only the leaf mapping is cleared.

### `protect_page(virt, new_flags)`
Walks to the leaf PTE (must already be a 4 KiB mapping — does not handle huge
pages), preserves the physical address, and rewrites the entry with new
permission flags via `PageTableEntry::make_page`, then invalidates the TLB entry.

### `translate(virt)`
Read-only walk that returns the physical address `virt` maps to, correctly
handling early termination at 1 GiB or 2 MiB huge leaves (masking the
appropriate offset into the huge page) or the full 4 KiB path. Returns `0` if
any level is not present.

### `Paging Errors`
* Success = 0
* OutOfMemory = 1
* AlreadyMapped = 2
* NotMapped = 3
* InvalidAlignment = 4
* InvalidContext = 5
* InvalidAddress = 6
* HardwareFault = 7    

---

## 4. SLUB Allocator (`slub.cpp`)

General-purpose kernel heap allocator (`kmalloc` / `kfree`) built on top of
`PMM::alloc_pages` / `PMM::free_pages`, implementing a simplified SLUB
(single page per slab, embedded freelist) design.

### `KmemCache`

One cache per size class. Constructed with a target `size`, which is rounded up
to at least `sizeof(void*)` (room for the embedded freelist pointer) and then to
an 8-byte boundary, stored as `obj_size`.

**`format_new_slab(header, page_virtual_addr)`**
Turns a fresh physical page (accessed via its HHDM virtual address) into a slab:
- Writes a `SlabHeader` at the start of the page: magic number `0x51AB51AB`,
  owning cache pointer, `inuse = 0`, and null intrusive slab-list links.
- Places the first object after the header, aligned to `min(obj_size, 64)` bytes
  (cacheline/SIMD friendly for small objects).
- Computes `max_objs` from the space actually available after that alignment
  padding.
- Chains every object to the next via an embedded freelist pointer
  (`get_freepointer`), terminating with `nullptr`.

**`alloc()`**
- Takes the head of `partial_list` (slabs with at least one free object) or, if
  none exist, asks the PMM for a fresh page and formats it as a new slab, pushed
  to the front of `partial_list`.
- Pops the head of the slab's freelist, increments `inuse`.
- If that empties the slab's freelist, unlinks the slab from `partial_list` (it's
  now full and untracked until something is freed back into it).

**`free(object)`**
- Locates the owning `SlabHeader` by masking `object`'s address down to the page
  boundary; verifies the magic number and that `header->cache == this`
  (mismatch → `kernel_panic`, catches cross-cache frees).
- Pushes `object` back onto the slab's embedded freelist, decrements `inuse`.
- If the slab transitions from **full → partial** (`inuse == max_objs - 1`
  after the decrement... i.e. was full before), re-links it into
  `partial_list`.
- If the slab becomes **completely empty** (`inuse == 0`), unlinks it from
  `partial_list` and returns the underlying physical page to the PMM via
  `PMM::free_pages`.

`acquire_lock` / `release_lock` are present as no-op hooks — the allocator is
currently **not** SMP-safe; a real lock (spinlock/IRQ-safe lock) needs to be
substituted here before use on multiple cores or with interrupt-context
allocation.

### Global `kmalloc` caches

`initialize_slub()` builds `NUM_KMALLOC_CACHES` power-of-two-sized caches
starting at `MIN_KMEM_CACHE_SIZE` and doubling, placed in a static pool
(`cache_memory_pool`) and constructed in-place with placement `new`.

`get_kmalloc_cache(size)` maps a requested size to the right cache index using
`__builtin_clzll` to find the position of the highest set bit of `size - 1`
(i.e. `ceil(log2(size))`), offset so that `MIN_KMEM_CACHE_SIZE` lands on index 0.

### `kmalloc(size)`
- `size == 0` → `nullptr`.
- If a matching cache exists (size ≤ largest `kmalloc` cache class), delegate to
  `KmemCache::alloc()`.
- Otherwise it's a **large allocation**: bypasses SLUB and asks the PMM directly
  for `ceil(size / PAGE_SIZE) + 1` pages — the extra page stores metadata (a
  `0xDEADBEEF` magic and the page count) so `kfree` can recover the allocation
  size later. The pointer returned to the caller points *after* that metadata
  page.

### `kfree(ptr)`
- `nullptr` → no-op.
- Masks `ptr` down to its containing page and inspects it as a `SlabHeader`. If
  the magic matches `0x51AB51AB` **and** the cache pointer matches one of the
  known `kmalloc_caches`, it's treated as a SLUB allocation and routed to
  `header->cache->free(ptr)`.
- Otherwise, it's assumed to be a large allocation: looks one page *before*
  `ptr`'s page for the `0xDEADBEEF` metadata magic and page count, then returns
  those pages to the PMM via `PMM::free_pages`.
- If neither the SLUB magic nor the large-allocation magic is found, calls
  `kernel_panic("Invalid free detected in kfree!\n")` — this is a heuristic
  double-free / corruption / invalid-pointer detector, not a guarantee (a stray
  write that happens to reproduce `0x51AB51AB` or `0xDEADBEEF` at the right
  offset would fool it).

---

## 5. Shared Definitions (`mm_defs.hpp`)

Central header included by every other memory-management translation unit.
`namespace mem`.

### Core types
- `PhysAddr`, `VirtAddr`, `usize` — all `uint64_t` aliases, used to keep physical
  vs. virtual addresses distinguishable in signatures even though they're the
  same underlying width.

### Constants

| Constant | Value | Meaning |
|---|---|---|
| `HHDM_BASE` | `0xFFFF800000000000` | Base virtual address of the Higher-Half Direct Map |
| `EARLY_BOOT_MAP_LIMIT` | `0x40000000` (1 GiB) | Extent of the boot-time identity/HHDM huge-page map; bump allocations must stay under this until `PagingBackend::initialize` extends HHDM coverage |
| `BUDDY_MAX_ORDER` | `10` | Largest buddy block = `PAGE_SIZE * 2^10` = 4 MiB |
| `NUM_KMALLOC_CACHES` | `8` | Number of power-of-two `kmalloc` size classes |
| `PAGE_SHIFT` | `12` | log2 of page size |
| `PAGE_SIZE` | `4096` (4 KiB) | Standard page size used for PMM/buddy/slab accounting |
| `PAGE_SIZE_2M` | `2 MiB` | Huge page size (PD-level) |
| `PAGE_SIZE_1G` | `1 GiB` | Huge page size (PDPT-level) |
| `PAGE_MASK` | `PAGE_SIZE - 1` | Alignment mask for 4 KiB pages |
| `PFN_MASK` | `0x000F'FFFF'FFFF'F000` | Masks the physical frame number out of a raw page-table entry |
| `INDEX_MASK` | `0x1FF` | 9-bit mask for extracting a single page-table level index |
| `OFFSET_MASK` | `0xFFF` | 12-bit mask for the in-page byte offset |
| `PT_ENTRY_COUNT` | `512` | Entries per page table level |

`mem_regions[5]` provides human-readable names (`"Available"`, `"Reserved"`,
`"ACPI Reclaimable"`, `"NVS"`, `"Bad RAM"`) indexed by
`multiboot_mmap_entry::type - 1`, used by `PMM::initialize_buddy`'s logging.

### `PageFlags`

A `uint32_t`-backed scoped enum of independent bits (`Read`, `Write`, `Execute`,
`User`, `Global`, `WriteThrough`, `NoCache`, `Huge2M`, `Huge1G`), with the usual
bitwise operators (`|`, `&`, `~`, `|=`, `&=`) defined so it behaves like a proper
flag set, plus `has_flag(flags, flag)` for containment checks.

Convenience presets combine common bit patterns:
- `KernelRO` / `KernelRW` / `KernelRX` / `KernelRWX` — kernel-only mappings with
  no `User` bit.
- `UserRO` / `UserRW` / `UserRX` — same, with `User` set.
- `MMIO` — `Read | Write | NoCache`, for device memory.

These are the flags consumed by `PagingBackend::map_page` / `protect_page`
(see §3) to decide permission bits, huge-page level, and cacheability when
building page-table entries.

### Alignment helpers

`page_align_up` / `page_align_down` / `is_page_aligned` operate on `VirtAddr` and
round to/check against `PAGE_SIZE` (4 KiB) boundaries. Note: `pmm.cpp` uses its
own `align_up`/`align_down` (from `lib/mem_util.hpp`, generic over any alignment)
rather than these page-specific helpers — both exist in the codebase and callers
should be consistent about which is appropriate (page-only vs. arbitrary
alignment).

### Placement `new`

Global placement `operator new(size_t, void*)` / `operator new[](size_t, void*)`
are defined so kernel code can construct objects at a specific address (used by
`mem::initialize_slub` to construct `KmemCache` instances inside the static
`cache_memory_pool`). The ordinary throwing/allocating forms of `operator new` /
`operator new[]` are explicitly `= delete`d — kernel code cannot accidentally
pull in a hosted heap allocator; all dynamic allocation must go through
`kmalloc`/`KmemCache`/`PMM` explicitly.

---

## 6. Cross-Component Invariants

- **HHDM is load-bearing everywhere.** `PMM`, `KmemCache`, and `PagingBackend`
  all convert between physical addresses and usable pointers via
  `phys + HHDM_BASE`. This is only valid after `PagingBackend::initialize` has
  mapped all of RAM into the HHDM — before that, only memory below
  `EARLY_BOOT_MAP_LIMIT` (covered by `boot.asm`'s 1 GiB huge-page bootstrap map)
  is safely dereferenceable.
- **Allocator layering:** `kmalloc`/`kfree` → `KmemCache` (small, fixed-size,
  slab-based) or directly → `PMM::alloc_pages`/`free_pages` (large allocations)
  → bump allocator (only pre-buddy-init) or buddy allocator (steady state).
- **One-way switch:** `PMM::initialized_buddy` never resets to `false`; once the
  buddy allocator takes over, `alloc_pages_bump` is permanently dead and all
  page-granular allocation goes through the buddy path.
- **Page size assumption:** the entire stack assumes a uniform 4 KiB `PAGE_SIZE`
  for bump/buddy/slab accounting, with separate explicit handling for 2 MiB and
  1 GiB huge pages only at the paging layer.