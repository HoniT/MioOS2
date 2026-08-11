// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef TSS_HPP
#define TSS_HPP

#include <stdint.h>

#define DF_STACK_PAGES 1
#define NMI_STACK_PAGES 1
#define MC_STACK_PAGES 1

namespace arch
{
    struct tss_ent_t {
        uint32_t rsrvd_1;
        uint64_t rsp0;
        uint64_t rsp1;
        uint64_t rsp2;
        uint32_t rsrvd_2;
        uint32_t rsrvd_3;
        uint64_t ist1;
        uint64_t ist2;
        uint64_t ist3;
        uint64_t ist4;
        uint64_t ist5;
        uint64_t ist6;
        uint64_t ist7;
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
