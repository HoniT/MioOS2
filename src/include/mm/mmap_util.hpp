// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef MMAP_UTIL_HPP
#define MMAP_UTIL_HPP

#include <mm/mm_defs.hpp>
#include <boot/uefi.hpp>
#include <boot/multiboot.hpp>

/// @brief A generic struct that both BIOS and EFI entries map into
struct UnifiedMemoryEntry {
    mem::PhysAddr addr;
    size_t len;
    bool is_usable;

    uint32_t type;
    bool is_uefi;
};

// MMAP Iterator class
class MmapIterator {
public:
    MmapIterator(multiboot_tag* tag) : tag(tag) {}

    // Accepts a lambda callback to process each memory region
    template<typename Func>
    void for_each(Func callback) {
        if (!tag) return;

        if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
            auto* mmap = (multiboot_tag_mmap*)tag;
            uint8_t* mmap_start = (uint8_t*)mmap->entries;
            uint8_t* mmap_end   = (uint8_t*)mmap + mmap->size;
            
            for (uint8_t* ptr = mmap_start; ptr < mmap_end; ptr += mmap->entry_size) {
                auto* ent = (multiboot_mmap_entry*)ptr;
                UnifiedMemoryEntry uent = {
                    ent->addr, 
                    ent->len, 
                    (ent->type == MULTIBOOT_MEMORY_AVAILABLE),
                    ent->type,
                    false
                };
                callback(uent);
            }
        } 
        else if (tag->type == MULTIBOOT_TAG_TYPE_EFI_MMAP) {
            auto* mmap = (multiboot_tag_efi_mmap*)tag;
            uint8_t* mmap_start = mmap->efi_mmap;
            uint8_t* mmap_end   = (uint8_t*)mmap + mmap->size;
            
            for (uint8_t* ptr = mmap_start; ptr < mmap_end; ptr += mmap->descriptor_size) {
                auto* ent = (efi_memory_descriptor*)ptr;
                UnifiedMemoryEntry uent = {
                    ent->physical_start, 
                    ent->number_of_pages * 4096, // UEFI pages are strictly 4KB
                    (ent->type == EfiConventionalMemory || 
                        ent->type == EfiBootServicesCode ||
                        ent->type == EfiBootServicesData),
                    ent->type,
                    true
                };
                callback(uent);
            }
        }
    }

private:
    multiboot_tag* tag;
};

#endif // MMAP_UTIL_HPP
