// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Interrupt Descriptor Table
// ========================================

#include <arch/interrupts/idt.hpp>
#include <arch/interrupts/pic.hpp>
#include <lib/mem_util.hpp>
#include <graphics/kernel_gui.hpp>
#include <kernel_panic.hpp>

using namespace arch;

bool IDT::initialized = false;

idtr_t IDT::idtr;
idt_gate_desc_t IDT::gates[IDT_ENTRIES];

isr_t isr_stub_table[32] = {
    isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7,
    isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15,
    isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
    isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
};

isr_t irq_stub_table[16] = {
    irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7, 
    irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15
};

bool IDT::early_initialize() {
    idtr.size = IDT_ENTRIES * sizeof(idt_gate_desc_t) - 1;
    idtr.base = (uint64_t)gates;
    memset(&gates, 0, sizeof(idt_gate_desc_t) * IDT_ENTRIES);

    // Remaping the PIC
    PIC_8259A::remap(0x20, 0x28);

    for(int i = 0; i < 32; i++)
        set_gate(&gates[i], (uint64_t)isr_stub_table[i], 0x08, 0, 0xE, 0);

    for(int i = 0; i < IRQ_QUANTITY; i++)
        set_gate(&gates[FIRST_IRQ_IDX + i], (uint64_t)irq_stub_table[i], 0x08, 0, 0xE, 0);

    idt_flush(&idtr);

    kprintf(gui::PrintTypes::LOG_INFO, "Early initialized the IDT\n");
    return true;
}

bool IDT::initialize() {
    // Reconfigure the 32 CPU exception gates with their proper DPL/IST/type
    for(int i = 0; i < 32; i++) {
        uint8_t type = 0xE;
        uint8_t dpl = 0;
        uint8_t ist = 0;

        if(i == 1 || i == 3) type = 0xF;   // #DB, #BP trap gate
        if(i == 3) dpl = 3;

        // Dedicated IST stacks (defined in TSS)
        if(i == 8) ist = 1;
        if(i == 2) ist = 2;
        if(i == 18) ist == 3;

        set_gate(&gates[i], (uint64_t)isr_stub_table[i], 0x08, ist, type, dpl);
    }

    for(int i = 0; i < IRQ_QUANTITY; i++)
        set_gate(&gates[FIRST_IRQ_IDX + i], (uint64_t)irq_stub_table[i], 0x08, 0, 0xE, 0);

    // Just making these present without real handlers YET
    for(int i = 0; i < RESERVED_QUANTITY; i++)
        set_gate(&gates[RESERVED_FIRST_IDX + i], (uint64_t)isr_reserved_table[i], 0x08, 0, 0xE, 0);

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

extern "C" void isr_handler(interrupt_registers_t* regs) {
    // Just a kernel panic for any ISR for now
    if(regs->interr_no < FIRST_IRQ_IDX) {
        kernel_panic(exception_messages[regs->interr_no], regs);
    }
}

extern "C" void isr_unhandled_handler(interrupt_registers_t* regs) {
    // Vectors 0x30-0xFF are reserved for future IRQ/MSI routing
    kprintf(gui::PrintTypes::LOG_ERROR, "Unhandled interrupt on vector %u\n", regs->interr_no);
}

static void* irq_routines[IRQ_QUANTITY] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

void IDT::irq_install_handler(int irq_num, void (*handler)(interrupt_registers_t* regs)) {
    if(irq_num < 0 || irq_num >= IRQ_QUANTITY) {
        kprintf(gui::PrintTypes::LOG_ERROR, "IDT: Attempted to install invalid IRQ %d\n", irq_num);
        return; 
    }
    
    // Installing IRQ
    irq_routines[irq_num] = (void*)handler;
}

void IDT::irq_uninstall_handler(int irq_num) {
    // Turrning IRQ back to 0
    irq_routines[irq_num] = 0;
}

extern "C" void irq_handler(interrupt_registers_t* regs) {
    void (*handler)(interrupt_registers_t* regs);

    // Getting value from irq_routines
    handler = (void (*)(interrupt_registers_t*))irq_routines[regs->interr_no - FIRST_IRQ_IDX];
    if(handler) handler(regs);

    PIC_8259A::send_eoi(regs->interr_no);
}
