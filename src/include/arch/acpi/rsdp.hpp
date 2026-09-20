// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef RSDP_HPP
#define RSDP_HPP

#include <arch/acpi/acpi.hpp>
#include <util/list.hpp>

namespace acpi
{
    struct sdp_descriptor {
        char signature[8];
        uint8_t checksum;
        char oemid[6];
        uint8_t revision;
        uint32_t rsdt_address;

        // ACPI 2.0+ extended fields (XSDP)
        uint32_t length;
        uint64_t xsdt_address;
        uint8_t extended_checksum;
        uint8_t reserved[3];
    } __attribute__((packed));

    struct rsdt_t : acpi_header_t {
        uint32_t pointers[];
    } __attribute__((packed));

    struct xsdt_t : acpi_header_t {
        uint64_t pointers[];
    } __attribute__((packed));



    /// @brief Represents both RSDP and XSDP depending on the revision
    class SystemDescriptionPointer {
    private:
        static sdp_descriptor* sdp;
        /// @brief Scans memory for the SDP
        /// @param start_phys Start PHYSICAL address of the region
        /// @param end_phys End PHYSICAL address of the region
        static sdp_descriptor* scan_memory_for_sdp(uintptr_t start_phys, uintptr_t end_phys);

        static bool is_valid_sdt_ent(acpi_header_t* table);

        static inline bool acpi_reclaimed = false;
        
    public:
        /// @brief Checks if a SDP is valid
        static bool is_valid_sdp(const sdp_descriptor* sdp);

        // Finds the RSDP/XSDP
        static sdp_descriptor* find_sdp(void* mb2_info);

        /// @brief Itterates through the XSDT/RSDT untill a table matches the given signature. USUALLY USE THE APIs DEFINED IN ACPI.CPP INSTEAD
        /// @return The virtual address of the table, or nullptr if no table found
        static acpi_header_t* find_table_by_signature(char signature[4]);

        /// @brief Parses the SDT and saves all tables in a given list
        static void parse_sdt(util::List<acpi_header_t*>& list);

        static inline void mark_acpi_reclaimed() { acpi_reclaimed = true; }
    };
} // namespace acpi


#endif // RSDP_HPP
