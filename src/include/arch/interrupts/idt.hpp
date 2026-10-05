// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef IDT_HPP
#define IDT_HPP

#include <stdint.h>

#define IDT_ENTRIES 256

namespace arch
{
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

    class IDT {
    private:
        idt_gate_desc_t gates[IDT_ENTRIES];
        idtr_t idtr;

    public:
        bool initialized;
        bool early_initialize();
        /// @brief Full init: populate all 256 IDT entries (correct DPL/IST/gate
        /// type per exception, reserved vectors covered by a default handler)
        bool initialize();

        void set_gate(idt_gate_desc_t* gate, const uint64_t base, const uint16_t selector, 
                             const uint8_t ist, const uint8_t type, const uint8_t dpl);
    };

    extern "C" void idt_flush(idtr_t* idtr);

} // namespace arch

#endif // IDT_HPP 
