// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// ACPI related definitions
// ========================================

#pragma once
#ifndef ACPI_HPP
#define ACPI_HPP

#include <stdint.h>
#include <util/list.hpp>

namespace acpi {

    /// @brief Starting header of every RSDT/XSDT table entry
    struct acpi_header_t {
        char signature[4];
        uint32_t length;
        uint8_t revision;
        uint8_t checksum;
        char oem_id[6];
        char oem_table_id[8];
        uint32_t oem_revision;
        uint32_t asl_compiler_id;
        uint32_t asl_compiler_revision;
    } __attribute__((packed));

    struct cpu_core_info_t {
        uint8_t acpi_cpu_id;
        uint8_t apic_id;
        uint32_t flags;
        bool is_bsp; // Is Bootstrap CPU
    };

    struct ioapic_info_t {
        uint8_t ioapic_id;
        uint32_t ioapic_address;
        uint32_t gsib;
    };

    struct ioapic_iso_t {
        uint8_t bus_source;
        uint8_t irq_source;
        uint32_t gsi;
        uint16_t flags;
    };

    struct lx2apic_t {
        uint32_t lx2apic_id;
        uint32_t flags;
        uint32_t acpi_id;
    };

    
    class ACPI {
    private:
        static util::List<acpi_header_t*> cached_acpi_headers;
        
        /// @brief Tables to look for and cache
        static inline constexpr char* needed_acpi_table_signatures[] = {
            "APIC", "HPET", "FACP", "MCFG", "DMAR", "IVRS"
        };
        
        static bool is_valid_sdt_ent(acpi_header_t* table);
        
    public:
        /// @brief Parses R/XSDT finds and caches needed tables
        static void parse_tables();

        /// @brief Gets an ACPI table by its signature, first checks the cache, if it's not found there it parses RSDT/XSDT
        /// @param sig 4 char signature of the acpi table
        static acpi_header_t* get_table_by_signature(char sig[4]);
    };

}; // namespace acpi

#endif // ACPI_HPP
