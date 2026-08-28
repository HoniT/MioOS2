// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Interrupt Descriptor Table
// ========================================

#include <arch/interrupts/idt.hpp>
#include <arch/interrupts/pic.hpp>
#include <arch/interrupts/lapic.hpp>
#include <lib/mem_util.hpp>
#include <graphics/kernel_gui.hpp>
#include <kernel_panic.hpp>
#include <cpu.hpp>

using namespace arch;

bool IDT::initialized = false;

idtr_t IDT::idtr;
idt_gate_desc_t IDT::gates[IDT_ENTRIES];

extern "C" isr_t cpu_isr_stub_table[CPU_IRQ_NUM]; 
extern "C" isr_t hw_isr_stub_table[HW_IRQ_NUM];

// Universal handler array for all 256 vectors
static void* interrupt_handlers[IDT_ENTRIES] = {0};

bool IDT::early_initialize() {
    idtr.size = IDT_ENTRIES * sizeof(idt_gate_desc_t) - 1;
    idtr.base = (uint64_t)gates;
    memset(&gates, 0, sizeof(idt_gate_desc_t) * IDT_ENTRIES);

    // Remaping the PIC
    PIC_8259A::remap(0x20, 0x28);

    for(int i = 0; i < CPU_IRQ_NUM; i++)
        set_gate(&gates[i], (uint64_t)cpu_isr_stub_table[i], 0x08, 0, 0xE, 0);

    for(int i = CPU_IRQ_NUM; i < IDT_ENTRIES; i++)
        set_gate(&gates[i], (uint64_t)hw_isr_stub_table[i - CPU_IRQ_NUM], 0x08, 0, 0xE, 0);

    idt_flush(&idtr);

    kprintf(gui::PrintTypes::LOG_INFO, "Early initialized the IDT\n");
    return true;
}

bool IDT::initialize() {
    // Reconfigure the 32 CPU exception gates with their proper DPL/IST/type
    for(int i = 0; i < CPU_IRQ_NUM; i++) {
        uint8_t type = 0xE;
        uint8_t dpl = 0;
        uint8_t ist = 0;

        if(i == 1 || i == 3) type = 0xF;   // #DB, #BP trap gate
        if(i == 3) dpl = 3;

        // Dedicated IST stacks (defined in TSS)
        if(i == 8) ist = 1;
        if(i == 2) ist = 2;
        if(i == 18) ist == 3;

        set_gate(&gates[i], (uint64_t)cpu_isr_stub_table[i], 0x08, ist, type, dpl);
    }
    
    // Spurious IRQs
    set_gate(&gates[0xFF], (uint64_t)spurious_irq_handler, 0x08, 0, 0xE, 0);

    idt_flush(&idtr);
    
    initialized = true;
    kprintf(gui::PrintTypes::LOG_INFO, "Fully initialized the IDT (%u vectors)\n", IDT_ENTRIES);
    return true;
}

void IDT::set_gate(idt_gate_desc_t* gate, const uint64_t base, const uint16_t selector, 
            const uint8_t ist, const uint8_t type, const uint8_t dpl) {
    if(gate == nullptr || ist > 0b111 || dpl > 0b11 || type > 0b1111) {
        kernel_panic("IDT: Invalid parameters passed to set_gate\n");
    }

    gate->offset_1 = (uint16_t)base;
    gate->offset_2 = (uint16_t)(base >> 16);
    gate->offset_3 = (uint32_t)(base >> 32);

    gate->dpl = dpl;
    gate->type = type;
    gate->ist = ist;
    gate->selector = selector;

    gate->present = 1;
    gate->zero = 0;
}

void IDT::register_interrupt_handler(uint8_t vector, void (*handler)(interrupt_registers_t* regs)) {
    interrupt_handlers[vector] = (void*)handler;
}

void IDT::unregister_interrupt_handler(uint8_t vector) {
    interrupt_handlers[vector] = 0;
}


extern "C" void spurious_irq_handler(interrupt_registers_t* regs) {
    // Doing absolutely nothing. We DO NOT send an EOI
}

extern "C" void cpu_irq_handler(interrupt_registers_t* regs) {
    if(regs->interr_no < CPU_IRQ_NUM) {
        // Just a kernel panic for now
        kernel_panic(exception_messages[regs->interr_no], regs);
    }
}

extern "C" void hw_irq_handler(interrupt_registers_t* regs) {
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
