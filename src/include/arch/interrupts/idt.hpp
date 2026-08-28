// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef IDT_HPP
#define IDT_HPP

#include <stdint.h>

#define IDT_ENTRIES 256
#define CPU_IRQ_NUM 32
#define HW_IRQ_NUM 224
typedef void (*isr_t)();

namespace arch
{
    // Exeption messages
    constexpr const char* exception_messages[CPU_IRQ_NUM] = {
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

        static void register_interrupt_handler(uint8_t vector, void (*handler)(interrupt_registers_t* regs));
        static void unregister_interrupt_handler(uint8_t vector);
    };

    extern "C" void idt_flush(idtr_t* idtr);
    /// @brief CPU triggered (0-31) IRQ handler
    extern "C" void cpu_irq_handler(interrupt_registers_t* regs);
    /// @brief Hardware triggered (32-255) IRQ handler
    extern "C" void hw_irq_handler(interrupt_registers_t* regs);
    /// @brief Spurious (typically Vector 255) IRQ handler
    extern "C" void spurious_irq_handler(interrupt_registers_t* regs);

} // namespace arch

#endif // IDT_HPP 
