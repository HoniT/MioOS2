// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef INTERRUPTS_HPP
#define INTERRUPTS_HPP

#include <stdint.h>

#define CPU_IRQ_NUM 32
#define HW_IRQ_NUM 224
typedef void (*isr_t)();

namespace arch
{
    /// @brief State of the CPU when an interrupt or exception fires
    struct interrupt_registers_t {
        uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
        uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;

        uint64_t interr_no; // Interrupt number
        uint64_t err_code;  // Error code (or dummy 0 if CPU didn't push one)

        uint64_t rip, cs, rflags, rsp, ss; // Pushed by CPU
    } __attribute__((packed));

    void register_interrupt_handler(uint8_t vector, void (*handler)(interrupt_registers_t* regs));
    void unregister_interrupt_handler(uint8_t vector);

    /// @brief CPU triggered (0-31) IRQ handler
    extern "C" void cpu_irq_handler(interrupt_registers_t* regs);
    /// @brief Hardware triggered (32-255) IRQ handler
    extern "C" void hw_irq_handler(interrupt_registers_t* regs);
    /// @brief Spurious (typically Vector 255) IRQ handler
    extern "C" void spurious_irq_handler(interrupt_registers_t* regs);

    // Exeption messages
    extern const char* exception_messages[CPU_IRQ_NUM];

} // namespace arch

#endif // INTERRUPTS_HPP
