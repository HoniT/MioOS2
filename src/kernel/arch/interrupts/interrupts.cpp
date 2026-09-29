// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Software level interrupt handling & routing
// ========================================

#include <arch/interrupts/interrupts.hpp>
#include <arch/interrupts/pic.hpp>
#include <arch/interrupts/lapic.hpp>
#include <kernel_ui.hpp>
#include <kernel_panic.hpp>
#include <cpu.hpp>

const char* arch::exception_messages[CPU_IRQ_NUM] = {
    "Divide Error (#DE)",
    "Debug Exception (#DB)",
    "NMI Interrupt",
    "Breakpoint (#BP)",
    "Overflow (#OF)",
    "BOUND Range Exceeded (#BR)",
    "Invalid Opcode (Undefined Opcode) (#UD)",
    "Device Not Available (No Math Coprocessor) (#NM)",
    "Double Fault (#DF)",
    "Coprocessor Segment Overrun",
    "Invalid TSS (#TS)",
    "Segment Not Present (#NP)",
    "Stack-Segment Fault (#SS)",
    "General Protection (#GP)",
    "Page Fault (#PF)",
    "Reserved Exception",
    "x87 FPU Floating-Point Error (Math Fault) (#MF)",
    "Alignment Check (#AC)",
    "Machine Check (#MC)",
    "SIMD Floating-Point Exception (#XM)",
    "Virtualization Exception (#VE)",
    "Control Protection Exception (#CP)",
    "Reserved Exception",
    "Reserved Exception",
    "Reserved Exception",
    "Reserved Exception",
    "Reserved Exception",
    "Reserved Exception",
    "Reserved Exception",
    "Reserved Exception",
    "Reserved Exception",
    "Reserved Exception"
};

// Universal handler array for all 256 vectors
static void* interrupt_handlers[256] = {0};

void arch::register_interrupt_handler(uint8_t vector, void (*handler)(arch::interrupt_registers_t* regs)) {
    interrupt_handlers[vector] = (void*)handler;
}

void arch::unregister_interrupt_handler(uint8_t vector) {
    interrupt_handlers[vector] = 0;
}

extern "C" void arch::spurious_irq_handler(arch::interrupt_registers_t* regs) {
    // Doing absolutely nothing. We DO NOT send an EOI
    (void)regs; // Suppress unused parameter warning
}

extern "C" void arch::cpu_irq_handler(arch::interrupt_registers_t* regs) {
    if(regs->interr_no < CPU_IRQ_NUM) {
        // Just a kernel panic for now
        kernel_panic(exception_messages[regs->interr_no], regs);
    }
}

extern "C" void arch::hw_irq_handler(arch::interrupt_registers_t* regs) {
    void (*handler)(interrupt_registers_t*) = 
        (void (*)(interrupt_registers_t*))interrupt_handlers[regs->interr_no];

    if(handler) {
        handler(regs);
    } else {
        kprintf(gui::PrintTypes::LOG_ERROR, "Unhandled hardware interrupt on vector %u\n", regs->interr_no);
    }

    // Universal EOI Logic
    if (PIC_8259A::disabled) {
        LAPIC::send_eoi();
    } else {
        // Fallback: If legacy PIC is still active, only vectors 32-47 need an EOI
        if (regs->interr_no >= 32 && regs->interr_no <= 47) {
            PIC_8259A::send_eoi(regs->interr_no - 32);
        }
    }
}
