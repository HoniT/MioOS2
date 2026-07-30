// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef GDT_HPP
#define GDT_HPP

#include <stdint.h>

#define GDT_SEGMENT_QUANTITY 7

namespace arch
{
    struct gdt_seg_t {
        uint16_t limit_lo;
        uint16_t base_lo;
        uint8_t base_mid;
        uint8_t access;
        uint8_t limit_hi : 4;
        uint8_t flags : 4;
        uint8_t base_hi;
    } __attribute__((packed));

    struct gdtr_t {
        uint16_t size;
        uint64_t base;
    } __attribute__((packed));

    class GDT {
    private:
        static gdt_seg_t segs[GDT_SEGMENT_QUANTITY];
        static gdtr_t gdtr;

    public:
        static bool initialized;
        /// @brief Initializes the GDT
        static void initialize();

        /// @brief Sets a GDT segment descriptor
        /// @param seg Segment to set
        /// @param base Base of the segment descriptor (IGNORED in 64-bit mode)
        /// @param limit Maximum addressable unit (IGNORED in 64-bit mode)
        /// @param access Access parameters byte
        /// @param flags Flags
        static void set_descriptor(gdt_seg_t* seg, const uint32_t base, const uint32_t limit, const uint8_t access, const uint8_t flags);
    };

    extern "C" void gdt_flush(gdtr_t* gdtr);
} // namespace arch

#endif // GDT_HPP 
