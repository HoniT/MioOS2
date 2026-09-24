// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef FADT_HPP
#define FADT_HPP

#include <stdint.h>

namespace acpi {
    class FADT {
    private:
        static uint16_t sci_irq;

    public:
        static void find_sci_irq();
        
        static inline uint16_t get_sci_irq() { return sci_irq; }
    };
}; // namespace acpi

#endif // FADT_HPP
