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

void kernel_panic(const char* msg, arch::interrupt_registers_t* regs) {
    kprintf("KERNEL PANIC: %s Interrupt No. %u\n", msg, regs->interr_no);

    kprintf("R15: 0x%x\n", regs->r15);
    kprintf("R14: 0x%x\n", regs->r14);
    kprintf("R13: 0x%x\n", regs->r13);
    kprintf("R12: 0x%x\n", regs->r12);
    kprintf("R11: 0x%x\n", regs->r11);
    kprintf("R10: 0x%x\n", regs->r10);
    kprintf("R9: 0x%x\n", regs->r9);
    kprintf("R8: 0x%x\n", regs->r8);

    kprintf("RDI: 0x%x\n", regs->rdi);
    kprintf("RSI: 0x%x\n", regs->rsi);
    kprintf("RBP: 0x%x\n", regs->rbp);
    kprintf("RBX: 0x%x\n", regs->rbx);
    kprintf("RDX: 0x%x\n", regs->rdx);
    kprintf("RCX: 0x%x\n", regs->rcx);
    kprintf("RAX: 0x%x\n", regs->rax);

    kprintf("Error code: 0x%x\n", regs->err_code);

    kprintf("RIP: 0x%x\n", regs->rip);
    kprintf("CS: 0x%x\n", regs->cs);
    kprintf("RFLAGS: 0x%x\n", regs->rflags);
    kprintf("RSP: 0x%x\n", regs->rsp);
    kprintf("SS: 0x%x\n", regs->ss);

    if(regs->err_code == 14) {
        uintptr_t cr2;
        asm volatile ("mov %%cr2, %0" : "=r" (cr2));
        kprintf("CR2: 0x%x\n", cr2);
    }
    
    cpu::CPU::haltloop();
}