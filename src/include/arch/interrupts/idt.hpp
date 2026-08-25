// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef IDT_HPP
#define IDT_HPP

#include <stdint.h>

#define IDT_ENTRIES 256
#define FIRST_IRQ_IDX 32
#define IRQ_QUANTITY 16
#define RESERVED_FIRST_IDX (FIRST_IRQ_IDX + IRQ_QUANTITY)
#define RESERVED_QUANTITY (IDT_ENTRIES - RESERVED_FIRST_IDX)
typedef void (*isr_t)();

namespace arch
{
    // Exeption messages
    constexpr const char* exception_messages[] = {
        "Devide Error (#DE)",
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

    struct idt_gate_desc_t {
        uint16_t offset_1;
        uint16_t selector;
        uint8_t ist : 3;
        uint8_t rsrvd_1 : 5;
        uint8_t type : 4;
        uint8_t zero : 1;
        uint8_t dpl : 2;
        uint8_t present : 1;
        uint16_t offset_2;
        uint32_t offset_3;
        uint32_t rsrvd_2;
    } __attribute__((packed));

    struct idtr_t {
        uint16_t size;
        uint64_t base;
    } __attribute__((packed));

    #include <stdint.h>

    /// @brief State of the CPU when an interrupt or exception fires
    struct interrupt_registers_t {
        uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
        uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;

        uint64_t interr_no; // Interrupt number
        uint64_t err_code;  // Error code (or dummy 0 if CPU didn't push one)

        uint64_t rip, cs, rflags, rsp, ss; // Pushed by CPU
    } __attribute__((packed));

    class IDT {
    private:
        static idt_gate_desc_t gates[IDT_ENTRIES];
        static idtr_t idtr;

    public:
        static bool initialized;
        static bool early_initialize();
        /// @brief Full init: populate all 256 IDT entries (correct DPL/IST/gate
        /// type per exception, reserved vectors covered by a default handler)
        static bool initialize();

        static void set_gate(idt_gate_desc_t* gate, const uint64_t base, const uint16_t selector, 
            const uint8_t ist, const uint8_t type, const uint8_t dpl);

        void irq_install_handler(const int irq_num, void (*handler)(struct interrupt_registers_t* regs));
        void irq_uninstall_handler(const int irq_num);
    };

    extern "C" void idt_flush(idtr_t* idtr);
    extern "C" void isr_handler(interrupt_registers_t* regs);
    extern "C" void irq_handler(interrupt_registers_t* regs);
    extern "C" void isr_spurious_handler(interrupt_registers_t* regs);
    extern "C" void isr_unhandled_handler(interrupt_registers_t* regs);

    // Auto-generated in isr.asm
    extern "C" isr_t isr_reserved_table[RESERVED_QUANTITY];

    extern "C" {
        void isr0();
        void isr1();
        void isr2();
        void isr3();
        void isr4();
        void isr5();
        void isr6();
        void isr7();
        void isr8();
        void isr9();
        void isr10();
        void isr11();
        void isr12();
        void isr13();
        void isr14();
        void isr15();
        void isr16();
        void isr17();
        void isr18();
        void isr19();
        void isr20();
        void isr21();
        void isr22();
        void isr23();
        void isr24();
        void isr25();
        void isr26();
        void isr27();
        void isr28();
        void isr29();
        void isr30();
        void isr31();
        
        void irq0();
        void irq1();
        void irq2();
        void irq3();
        void irq4();
        void irq5();
        void irq6();
        void irq7();
        void irq8();
        void irq9();
        void irq10();
        void irq11();
        void irq12();
        void irq13();
        void irq14();
        void irq15();
    }
} // namespace arch

#endif // IDT_HPP 
