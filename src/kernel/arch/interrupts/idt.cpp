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

void IDT::set_gate(idt_gate_desc_t* gate, const uint64_t base, const uint16_t selector, 
            const uint8_t ist, const uint8_t type, const uint8_t dpl) {
    if(gate == nullptr) return;
    if(ist > 0b111) return;
    if(dpl > 0b11) return;
    if(type > 0b1111) return;

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

static void* irq_routines[IRQ_QUANTITY] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

void IDT::irq_install_handler(int irq_num, void (*handler)(interrupt_registers_t* regs)) {
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
