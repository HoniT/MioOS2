# Symmetric Multi-Processing (SMP) Subsystem

The SMP module manages the initialization, state allocation, and booting sequence for all secondary Application Processors (APs) in a multi-core environment, transitioning them from real mode to 64-bit long mode.

## Core Functions

### `cpu::alloc_ap_cpus()`
Iterates through the system's CPU topology (excluding the Bootstrap Processor) and dynamically allocates a `cpu::CPU` structure for each AP. It assigns sequential AP IDs and registers them in the `SystemTopology::ap_cpus` list.

### `cpu::ap_trampoline_setup(void* page)`
Prepares the low-memory execution environment required for waking up APs.
* Identity-maps a physical page with Read/Write/Execute permissions.
* Copies the 16-bit/32-bit assembly trampoline code into this page.
* Calculates and dynamically patches absolute physical addresses into the trampoline code for the 32-bit jump, 64-bit jump, base physical address, and the Global Descriptor Table (GDT) pointer.

### `cpu::smp_start_aps()`
Executes the standard INIT-SIPI-SIPI (Startup IPI) sequence to wake each AP sequentially.
* Captures the Bootstrap Processor's page table (`CR3`).
* In a loop for each AP, it patches CPU-specific data into the trampoline variables (Page Table, Kernel Stack, Entry Function `ap_entry`, and the CPU object pointer).
* Sends an `INIT` Inter-Processor Interrupt (IPI) via the Local APIC, followed by a 10ms delay.
* Sends two `SIPI` interrupts with the trampoline page vector, pausing briefly between them.
* Uses the atomic `ap_online_count` to pause execution until the current AP confirms it has successfully booted before modifying the trampoline parameters for the next AP.

### `ap_entry(cpu::CPU* cpu_obj)`
The primary 64-bit C++ entry point executed by each AP after successfully completing the assembly trampoline.
* Enables the No-Execute (NX) bit via the EFER MSR if supported.
* Initializes processor-specific structures: GDT, SLUB allocator, Task State Segment (TSS), Interrupt Descriptor Table (IDT), and Floating-Point Unit (FPU).
* Enables the AP's Local APIC.
* Increments the `ap_online_count` to signal the BSP that the core is ready.
* Enters an infinite `hlt` loop, waiting to service interrupts.

## Assembly Trampoline Implementation

The AP trampoline code is responsible for waking up a secondary processor in 16-bit Real Mode and transitioning it all the way to 64-bit Long Mode before handing execution over to the C++ kernel. 

### Transition Phases
1. **16-bit Real Mode (`trampoline_start`):** Synchronizes segment registers using the SIPI vector code segment. It loads a 32-bit temporary GDT, enables Protected Mode via `CR0`, and performs a far jump into 32-bit mode.
2. **32-bit Protected Mode (`trampoline32`):** Sets up data segments, enables Physical Address Extension (PAE) in `CR4`, loads the BSP's page tables into `CR3`, and enables Long Mode in the EFER MSR. Paging is then enabled, followed by a far jump into 64-bit mode.
3. **64-bit Long Mode (`trampoline64`):** Loads the pre-allocated kernel stack and passes the AP's specific `cpu::CPU*` object pointer into the `rdi` register (System V ABI first argument). Finally, it calls the `ap_entry` C++ function.
