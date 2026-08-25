// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef PIC_HPP
#define PIC_HPP

#include <stdint.h>

#define PIC_MASTER_COMMAND 0x20
#define PIC_MASTER_DATA    0x21
#define PIC_SLAVE_COMMAND 0xA0
#define PIC_SLAVE_DATA    0xA1

#define ICW1_INIT 0x10 // Initialization
#define ICW1_ICW4 0x01 // Indicates that ICW4 will be present
#define ICW4_8086 0x01 // 8086/88 (MCS-80/85) mode

#define CASCADE_IRQ 2

#define PIC_EOI 0x20 // End of Interrupt signal code

namespace arch
{
    class PIC_8259A {
    public:
        static bool remaped;
        static bool disabled;
        /// @brief Remaps the PIC
        /// @param offset1 Vector offset for master PIC
        /// @param offset2 Vector offset for slave PIC
        static void remap(int offset1, int offset2);

        /// @brief Sends an End of Interrupt signal for a given IRQ
        /// @param irq IRQ number
        static void send_eoi(const uint8_t irq);

        /// @brief Makes it ignore a request for an IRQ
        static void mask_irq(uint8_t irq);
        /// @brief Makes it remove the mask for an IRQ
        static void unmask_irq(uint8_t irq);

        /// @brief Disables the 8259A PIC
        static void disable();
    };
} // namespace arch

#endif // PIC_HPP
