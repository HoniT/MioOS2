// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Kernel panic handler
// ========================================

#include <kernel_panic.hpp>
#include <graphics/kernel_gui.hpp>
#include <cpu.hpp>

void kernel_panic(const char* msg) {
    kprintf(gui::PrintTypes::LOG_INFO, "KERNEL PANIC: %s\n", msg);
    cpu::CPU::haltloop();
}
