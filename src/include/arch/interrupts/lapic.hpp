// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef LAPIC_HPP
#define LAPIC_HPP

#include <stdint.h>

namespace arch
{
    // LAPIC Register Offsets (in bytes)
        constexpr uint32_t REG_ID           = 0x0020;
        constexpr uint32_t REG_VERSION      = 0x0030;
        constexpr uint32_t REG_TPR          = 0x0080; // Task Priority Register
        constexpr uint32_t REG_EOI          = 0x00B0; // End Of Interrupt
        constexpr uint32_t REG_SIVR         = 0x00F0; // Spurious Interrupt Vector Register
        
        // Local Vector Table (LVT) Registers
        constexpr uint32_t REG_LVT_TIMER    = 0x0320;
        constexpr uint32_t REG_LVT_LINT0    = 0x0350;
        constexpr uint32_t REG_LVT_LINT1    = 0x0360;
        constexpr uint32_t REG_LVT_ERROR    = 0x0370;

        // Configuration Flags
        constexpr uint32_t SIVR_ENABLE      = 0x0100; // Bit 8 enables the APIC
        constexpr uint32_t LVT_MASKED       = 0x10000; // Bit 16 masks the interrupt

    /// @brief Local Advanced Programmable Interrupt Controller
    class LAPIC {
    private:
        // Checks if the current CPU has APIC
        [[nodiscard]] static bool has_apic(); 

        static volatile uint32_t* lapic_virt_base;

    public:

        /// @brief Read a 32-bit value from a LAPIC register
        [[nodiscard]] static inline uint32_t read_reg(uint32_t offset) {
            if(!initialized) return 0xDEADBEEF; // Simple error code
            return lapic_virt_base[offset / 4];
        }

        /// @brief Write a 32-bit value to a LAPIC register
        static inline void write_reg(uint32_t offset, uint32_t value) {
            if(!initialized) return;
            lapic_virt_base[offset / 4] = value;
        }

        
        static bool initialized;
        /// @brief Initializes the Local APIC for the current CPU
        /// @param lapic_virt_base Local APIC base mapped to a virtual address. It needs to be Cache-Disabled and Write-Through.
        /// @return Success status
        static bool initialize(uint32_t* lapic_virt_base);

        static void send_eoi();
    };
} // namespace arch


#endif // LAPIC_HPP
