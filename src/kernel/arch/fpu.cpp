// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// FPU handling
// ========================================

#include <arch/fpu.hpp>
#include <cpu.hpp>
#include <graphics/kernel_gui.hpp>

uint32_t arch::X87_FPU::xsave_area_size;
uint32_t arch::X87_FPU::xsave_area_align;
bool arch::X87_FPU::initialized = false;

/// !! I'll leave this like this for now, because implementing actual xsave area allocation and things will be
/// not needed for now untill I have a working process/thread system working

void arch::X87_FPU::initialize() {
    // All control register bits are st up in CPU::init_features
    
    asm volatile("fninit");

    xsave_area_size = cpu::CPU::get_bsp_cpu().xsave_area_size;
    xsave_area_align = 64;

    initialized = true;
    kprintf(gui::LOG_INFO, "Initialized the x87 FPU\n");
}
