// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// SLUB allocator
// ========================================

#include <mm/slub.hpp>
#include <mm/pmm.hpp>
#include <graphics/kernel_gui.hpp>
#include <kernel_panic.hpp>
#include <lib/mem_util.hpp>

using namespace mem;

static KmemCache* kmalloc_caches[NUM_KMALLOC_CACHES];

// Helper to find the right generic cache based on requested size
static KmemCache* get_kmalloc_cache(size_t size) {
    if (size <= MIN_KMEM_CACHE_SIZE) return kmalloc_caches[0];
    
    // __builtin_clzll - number of consecutive zeros from MSB to first 1 
    int index = 64 - __builtin_clzll(size - 1) - 4;
    
    if (index >= 0 && index < NUM_KMALLOC_CACHES) {
        return kmalloc_caches[index];
    }
    return nullptr;
}

void KmemCache::acquire_lock() { }
void KmemCache::release_lock() { }


KmemCache::KmemCache(const char* cache_name, size_t size) 
    : name(cache_name), partial_list(nullptr) 
{
    // Minimum size must fit a pointer for the embedded freelist
    if (size < sizeof(void*)) {
        size = sizeof(void*);
    }
    
    // 8 byte align
    if (size % 8 != 0) {
        size += (8 - (size % 8));
    }
    
    obj_size = size;
}

void KmemCache::format_new_slab(SlabHeader* header, void* page_virtual_addr) {
    header->magic = 0x51AB51AB; // Safety magic number
    header->cache = this;
    header->inuse = 0;
    header->next_slab = nullptr;
    header->prev_slab = nullptr;

    // Align the first object to cacheline/SIMD bounds (up to 64 bytes)
    uintptr_t first_obj_addr = reinterpret_cast<uintptr_t>(page_virtual_addr) + sizeof(SlabHeader);
    size_t alignment = (obj_size < 64) ? obj_size : 64; 
    first_obj_addr = align_up(first_obj_addr, alignment);

    void* first_object = reinterpret_cast<void*>(first_obj_addr);
    header->freelist = first_object;

    // Recalculate max_objs accounting for alignment padding
    size_t available_space = PAGE_SIZE - (first_obj_addr - reinterpret_cast<uintptr_t>(page_virtual_addr));
    header->max_objs = available_space / obj_size;

    // Chain all objects together
    void* current_obj = first_object;
    for (uint32_t i = 0; i < header->max_objs - 1; i++) {
        void* next_obj = reinterpret_cast<uint8_t*>(current_obj) + obj_size;
        *get_freepointer(current_obj) = next_obj;
        current_obj = next_obj;
    }
    
    *get_freepointer(current_obj) = nullptr;
}

void* KmemCache::alloc() {
    acquire_lock();
    
    SlabHeader* target_slab = partial_list;

    // If we have no partial slabs, ask PMM for a fresh physical page
    if (!target_slab) {
        void* phys_ptr = PMM::alloc_pages();
        if (!phys_ptr) {
            release_lock();
            return nullptr; // Out of memory
        }

        uintptr_t virt_addr = reinterpret_cast<uintptr_t>(phys_ptr) + HHDM_BASE;
        target_slab = reinterpret_cast<SlabHeader*>(virt_addr);
        
        format_new_slab(target_slab, reinterpret_cast<void*>(virt_addr));
        
        // Add new slab to the partial list
        target_slab->next_slab = partial_list;
        target_slab->prev_slab = nullptr;
        if (partial_list) partial_list->prev_slab = target_slab;
        partial_list = target_slab;
    }

    // Pop the first free object off the embedded freelist
    void* allocated_object = target_slab->freelist;
    
    target_slab->freelist = *get_freepointer(allocated_object);
    target_slab->inuse++;

    // If the slab is completely full now, remove it from the partial list
    if (target_slab->freelist == nullptr) {
        partial_list = target_slab->next_slab;
        if (partial_list) partial_list->prev_slab = nullptr;
        target_slab->next_slab = nullptr;
        target_slab->prev_slab = nullptr;
    }

    release_lock();
    return allocated_object;
}

void KmemCache::free(void* object) {
    if (!object) return;

    // Finding the header
    uintptr_t page_base = reinterpret_cast<uintptr_t>((void*)align_down((uint64_t)object, PAGE_SIZE));
    SlabHeader* header = reinterpret_cast<SlabHeader*>(page_base);
    
    if (header->magic != 0x51AB51AB || header->cache != this) {
        kernel_panic("Freed object to wrong cache (in KmemCache::free)!\n");
        return; 
    }

    acquire_lock();

    // Push the object back onto the freelist
    *get_freepointer(object) = header->freelist;
    header->freelist = object;
    header->inuse--;

    // If it WAS completely full, it is now partial
    if (header->inuse == header->max_objs - 1) {
        header->next_slab = partial_list;
        header->prev_slab = nullptr;
        if (partial_list) partial_list->prev_slab = header;
        partial_list = header;
    }

    // If it is completely empty, give the page back to the PMM
    if (header->inuse == 0) {
        // Remove from the partial linked list
        if (header->prev_slab) {
            header->prev_slab->next_slab = header->next_slab;
        } else {
            partial_list = header->next_slab;
        }
        
        if (header->next_slab) {
            header->next_slab->prev_slab = header->prev_slab;
        }
        
        void* phys_ptr = reinterpret_cast<void*>(page_base - HHDM_BASE);
        PMM::free_pages(phys_ptr);
    }
    
    release_lock();
}

static uint8_t cache_memory_pool[sizeof(KmemCache) * NUM_KMALLOC_CACHES];

void mem::initialize_slub() {
    size_t target_size = MIN_KMEM_CACHE_SIZE;
    for (int i = 0; i < NUM_KMALLOC_CACHES; i++) {
        void* cache_ptr = &cache_memory_pool[i * sizeof(KmemCache)];
        kmalloc_caches[i] = new (cache_ptr) KmemCache("kmalloc", target_size);
        target_size *= 2;
    }

    kprintf(gui::LOG_INFO, "Initialized SLUB allocator with %u caches\n", NUM_KMALLOC_CACHES);
}

void* kmalloc(size_t size) {
    if (size == 0) return nullptr;
    
    // Find the right cache for this size
    KmemCache* cache = get_kmalloc_cache(size);
    if (cache) {
        return cache->alloc();
    }
    
    // If the request is > 2048 bytes, bypass SLUB and ask PMM directly
    // We allocate 1 extra page to store metadata (so kfree knows how many pages to free)
    size_t pages_needed = (size + PAGE_SIZE - 1) / PAGE_SIZE + 1;
    void* phys_ptr = PMM::alloc_pages(pages_needed);
    if (!phys_ptr) return nullptr;
    
    uintptr_t virt_addr = reinterpret_cast<uintptr_t>(phys_ptr) + HHDM_BASE;
    
    // Store metadata in the extra page
    uint32_t* meta_magic = reinterpret_cast<uint32_t*>(virt_addr);
    size_t* meta_pages = reinterpret_cast<size_t*>(virt_addr + sizeof(uint32_t));
    *meta_magic = 0xDEADBEEF; // Bypass magic number
    *meta_pages = pages_needed;
    
    return reinterpret_cast<void*>(virt_addr + PAGE_SIZE);
}

void kfree(void* ptr) {
    if (!ptr) return;

    uintptr_t virt_addr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t page_base = virt_addr & ~(PAGE_SIZE - 1);
    
    SlabHeader* header = reinterpret_cast<SlabHeader*>(page_base);
    
    bool is_slub_page = false;
    
    // Safe heuristic: Ensure magic matches before trusting cache pointer
    if (header->magic == 0x51AB51AB) {
        for (int i = 0; i < NUM_KMALLOC_CACHES; i++) {
            if (header->cache == kmalloc_caches[i]) {
                is_slub_page = true;
                break;
            }
        }
    }

    if (is_slub_page) {
        header->cache->free(ptr);
    } else {
        // Look back one page for our large allocation metadata
        uintptr_t meta_page = page_base - PAGE_SIZE;
        uint32_t* meta_magic = reinterpret_cast<uint32_t*>(meta_page);
        
        if (*meta_magic == 0xDEADBEEF) {
            size_t* meta_pages = reinterpret_cast<size_t*>(meta_page + sizeof(uint32_t));
            void* phys_ptr = reinterpret_cast<void*>(meta_page - HHDM_BASE);
            PMM::free_pages(phys_ptr, *meta_pages);
        } else {
            // I'll probably change this to handle it better later :)
            kernel_panic("Invalid free detected in kfree!\n");
        }
    }
}
