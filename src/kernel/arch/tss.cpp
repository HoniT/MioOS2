// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Task State Segment
// ========================================

#include <arch/tss.hpp>
#include <graphics/kernel_gui.hpp>

using namespace arch;

extern "C" uint8_t stack_top[];

bool TSS::initialized = false;
tss_ent_t TSS::tss_entry{0};

void TSS::initialize() {
    // Kernel stack
    uint64_t stack_ptr = (uint64_t)&stack_top;
    tss_entry.rsp0_lo = (uint32_t)(stack_ptr & 0xFFFFFFFF);
    tss_entry.rsp0_hi = (uint32_t)(stack_ptr >> 32);

    tss_entry.iopb = sizeof(tss_ent_t);

    tss_flush(0x28);

    kprintf(gui::PrintTypes::LOG_INFO, "Initialized the TSS for the BSP\n");
    initialized = true;
}
