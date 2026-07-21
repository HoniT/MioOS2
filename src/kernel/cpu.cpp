// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// x86_64 CPU helper methods
// ========================================

#include <cpu.hpp>
#include <graphics/kprint.hpp>

using namespace cpu;

[[noreturn]] void CPU::haltloop() {
    for(;;) {
        asm volatile("cli");
        asm volatile("hlt");
    }
}

void CPU::enable_interrupts() { asm volatile("sti"); }

void CPU::disable_interrupts() { asm volatile("cli"); }
