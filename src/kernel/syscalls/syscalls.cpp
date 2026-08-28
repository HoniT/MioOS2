// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Syscall handling
// ========================================

#include <syscalls/syscalls.hpp>
#include <cpu.hpp>
#include <arch/gdt.hpp>
#include <kernel_ui.hpp>

extern "C" const arch::syscall_fn_t syscall_table[] = {
    // ...
};

extern "C" const uint64_t __NR_syscall_max = (sizeof(syscall_table) / sizeof(syscall_table[0])) - 1;

void arch::syscall_msr_init() {
    // Enabling SCE
    uint64_t efer = cpu::CPU::read_msr(0xC0000080);
    efer |= (1ULL << 0);
    cpu::CPU::write_msr(0xC0000080, efer);

    // STAR
    uint64_t star = ((uint64_t)KERNEL_CODE << 32) |
                     (0x10ULL << 48);
    cpu::CPU::write_msr(0xC0000081, star);

    // LSTAR
    cpu::CPU::write_msr(0xC0000082, reinterpret_cast<uint64_t>(&syscall_entry));

    // FMASK
    uint64_t sfmask = (1ULL << 9) | (1ULL << 10) | (1ULL << 8) | (1ULL << 18);
    cpu::CPU::write_msr(0xC0000084, sfmask);

    kprintf(gui::LOG_INFO, "Initialized MSRs to handle syscalls\n");
}
