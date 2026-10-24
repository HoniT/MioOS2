// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// FPU handling
// ========================================

#include <arch/fpu.hpp>
#include <registry/system_topology_registry.hpp>
#include <kernel_ui.hpp>
#include <kernel_panic.hpp>

/// !! I'll leave this like this for now, because implementing actual xsave area allocation and things will be
/// not needed for now untill I have a working process/thread system working

void arch::FPU_X87::initialize() {
    // All control register bits are st up in CPU::init_features

    cpu::CPU* curr_cpu = cpu::CPU::get();
    if(!curr_cpu) kernel_panic("FPU::initialize: Couldnt get the current CPU core!\n");
    
    asm volatile("fninit");

    xsave_area_size = curr_cpu->cpuid_cache.xsave_area_size;
    xsave_area_align = 64;

    initialized = true;
    kprintf(gui::LOG_INFO, "Initialized the x87 FPU\n");
}
