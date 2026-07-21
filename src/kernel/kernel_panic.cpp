// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Kernel panic handler
// ========================================

#include <kernel_panic.hpp>
#include <graphics/kprint.hpp>
#include <cpu.hpp>

void kernel_panic(char* origin, const char* msg) {
    klogf(origin, "KERNEL PANIC: %s\n", msg);
    cpu::CPU::haltloop();
}
