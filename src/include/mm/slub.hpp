// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef SLUB_HPP
#define SLUB_HPP

#include <mm/mm_defs.hpp>
#include <util/spinlock.hpp>

#define MIN_KMEM_CACHE_SIZE 16
#define MAX_CACHES 256

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

        util::Spinlock cache_lock;

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

    /// @brief Initializes SLUB for the BSP
    void initialize_slub();
    /// @brief Initializes SLUB for an AP core
    void initialize_slub_ap();
} // namespace mem

// Global Memory API

/// @brief Allocate generic memory
void* kmalloc(size_t size);
/// @brief Free generic memory
void kfree(void* ptr);

#endif // SLUB_HPP