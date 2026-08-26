// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef IOAPIC_HPP
#define IOAPIC_HPP

#include <stdint.h>
#include <arch/acpi/acpi.hpp>

namespace arch
{
    // MMIO Offsets
    constexpr uint32_t IOREGSEL = 0x00;
    constexpr uint32_t IOWIN    = 0x10;

    // Internal Register Indices
    constexpr uint32_t IOAPICID  = 0x00;
    constexpr uint32_t IOAPICVER = 0x01;
    constexpr uint32_t IOREDTBL  = 0x10; // First Redirection Table Entry

    union rte_t
    {
        struct
        {
            uint64_t vector       : 8;
            uint64_t delvMode     : 3;
            uint64_t destMode     : 1;
            uint64_t delvStatus   : 1;
            uint64_t pinPolarity  : 1;
            uint64_t remoteIRR    : 1;
            uint64_t triggerMode  : 1;
            uint64_t mask         : 1;
            uint64_t reserved     : 39;
            uint64_t destination  : 8;
        };
        struct
        {
            uint32_t lowerDword;
            uint32_t upperDword;
        };
    };

    enum IRQDeliveryMode {
        Fixed = 0b000,
        Low = 0b001,
        SMI = 0b010,
        NMI = 0b100,
        INIT = 0b101,
        ExtINIT = 0b111
    };

    enum IRQDestinationMode {
        Physical = 0,
        Logical = 1
    };

    class IOAPIC {
    private:
        ioapic_info_t ioapic_info;
        volatile uint32_t* ioapic_virt_base;
        bool initialized;

    public:
        IOAPIC(ioapic_info_t ioapic_info) : ioapic_virt_base(nullptr), initialized(false), ioapic_info(ioapic_info) { }
        ~IOAPIC() { initialized = false; }
        
        /// @brief Helper to find the actual GSI for a legacy IRQ 
        static uint32_t get_gsi_for_irq(uint8_t irq, uint16_t& out_flags);
        
        uint32_t read_reg(uint32_t reg);
        void write_reg(uint32_t reg, uint32_t value);
        /// @brief Writes a 64-bit Redirection Tale Entry
        /// @param gsi The global system interrupt
        /// @param vector IRQ Vector
        /// @param dest Destination for the RTE (We will most commonly use physical detsination modes so the dest field must contain the apic id we want to send the signal to, 
        ///             but in the off chance dest_mod is set to Logical the dest field should contain the bitmap/cluster group)
        /// @param flags ISO flags
        /// @param masked If this IRQ will be masked out
        /// @param delv_mode Delivery mode- mostly `Fixed`
        /// @param dest_mode Destination mode- use only `Physical`, but Logical is supported aswell
        void write_rte(uint8_t gsi, uint8_t vector, uint8_t dest, uint16_t flags, bool masked, IRQDeliveryMode delv_mode = IRQDeliveryMode::Fixed, IRQDestinationMode dest_mode = IRQDestinationMode::Physical);
        
        bool initialize();
    };
    
} // namespace arch


#endif // IOAPIC_HPP
