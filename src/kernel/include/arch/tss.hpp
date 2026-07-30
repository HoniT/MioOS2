// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef TSS_HPP
#define TSS_HPP

#include <stdint.h>

namespace arch
{
    struct tss_ent_t {
        uint32_t rsrvd_1;
        uint32_t rsp0_lo;
        uint32_t rsp0_hi;
        uint32_t rsp1_lo;
        uint32_t rsp1_hi;
        uint32_t rsp2_lo;
        uint32_t rsp2_hi;
        uint32_t rsrvd_2;
        uint32_t rsrvd_3;
        uint32_t ist1_lo;
        uint32_t ist1_hi;
        uint32_t ist2_lo;
        uint32_t ist2_hi;
        uint32_t ist3_lo;
        uint32_t ist3_hi;
        uint32_t ist4_lo;
        uint32_t ist4_hi;
        uint32_t ist5_lo;
        uint32_t ist5_hi;
        uint32_t ist6_lo;
        uint32_t ist6_hi;
        uint32_t ist7_lo;
        uint32_t ist7_hi;
        uint32_t rsrvd_4;
        uint32_t rsrvd_5;
        uint16_t rsrvd_6;
        uint16_t iopb;
    } __attribute__((packed));

    class TSS {
    public:
        static bool initialized;
        /// @brief Initializes the TSS
        static void initialize();
        
        static tss_ent_t tss_entry;
    };

    extern "C" void tss_flush(uint16_t gdt_selector);
} // namespace arch


#endif // TSS_HPP
