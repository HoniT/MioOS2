// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef MADT_HPP
#define MADT_HPP

#include <arch/acpi/acpi.hpp>

#pragma region Structs

struct madt_t {
    sdt_header_t acpi_header;

    uint32_t lapic_address;
    uint32_t flags;

    uint8_t entries[];
} __attribute__((packed));

/// @brief Base of every MADT entry
struct madt_ent_t {
    uint8_t type;
    uint8_t length;
} __attribute__((packed));

struct madt_lapic_ent_t {
    madt_ent_t base;

    uint8_t acpi_cpu_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed));

struct madt_ioapic_ent_t {
    madt_ent_t base;

    uint8_t ioapic_id;
    uint8_t reserved;
    uint32_t ioapic_address;
    uint32_t gsib;
} __attribute__((packed));

struct madt_ioapic_iso_ent_t {
    madt_ent_t base;

    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t gsi;
    uint16_t flags;
} __attribute__((packed));

struct madt_ioapic_nmi_ent_t {
    madt_ent_t base;

    uint8_t nmi_source;
    uint8_t reserved;
    uint16_t flags;
    uint32_t gsi;
} __attribute__((packed));

struct madt_lapic_nmi_ent_t {
    madt_ent_t base;

    uint8_t acpi_cpu_id;
    uint16_t flags;
    uint8_t lint;
} __attribute__((packed));

struct madt_lapic_addr_ent_t {
    madt_ent_t base;

    uint16_t reserved;
    uint64_t address;
} __attribute__((packed));

struct madt_lx2apic_ent_t {
    madt_ent_t base;

    uint16_t reserved;
    uint32_t lx2apic_id;
    uint32_t flags;
    uint32_t acpi_id;
} __attribute__((packed));

#pragma endregion

namespace acpi
{
    class MADT {
    private:
        static madt_t* madt;
        
    public:
        /// @brief Parses the MADT entries and finds all of the CPU's cores and APIC's
        /// @return Success status
        static bool parse_madt();
    };
} // namespace acpi


#endif // MADT_HPP
