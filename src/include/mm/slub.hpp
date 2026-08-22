// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef SLUB_HPP
#define SLUB_HPP

#include <mm/mm_defs.hpp>

#define MIN_KMEM_CACHE_SIZE 16

namespace mem
{
    class KmemCache;

    /// @brief Placed at the very beginning of every 4KB page owned by SLUB
    struct SlabHeader {
        uint32_t magic; // Safely distinguishes SLUB pages from user memory
        KmemCache* cache;
        void* freelist;
        uint32_t inuse;
        uint32_t max_objs;
        SlabHeader* next_slab;
        SlabHeader* prev_slab;
    };

    /// @brief The manager for a specific object size (e.g., a 64-byte cache)
    class KmemCache {
    private:
        const char* name;
        size_t obj_size;
        SlabHeader* partial_list; // List of pages that are partially full

        // I just pasted this from the PMM to make the alloc/dealloc clearer.
        // When I'll implement SMP in the future I'll change this to have Per-CPU caches,
        // before then I just want a working SLUB allocator and kernel heap to continue with other things
        void acquire_lock();
        void release_lock();

        /// @brief Helper: Gets the embedded freelist pointer inside a free object
        inline void** get_freepointer(void* object) {
            return reinterpret_cast<void**>(object);
        }

        /// @brief Helper: Formats a brand new 4KB page into a SLUB page
        void format_new_slab(SlabHeader* header, void* page_virtual_addr);

    public:
        KmemCache(const char* cache_name, size_t size);
        
        void* alloc();
        void free(void* object);
        
        const char* get_name() const { return name; }
        size_t get_size() const { return obj_size; }
    };

    void initialize_slub();
} // namespace mem

// Global Memory API

/// @brief Allocate generic memory
void* kmalloc(size_t size);
/// @brief Free generic memory
void kfree(void* ptr);

#endif // SLUB_HPP