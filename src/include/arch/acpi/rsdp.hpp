// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef RSDP_HPP
#define RSDP_HPP

#include <stdint.h>

struct rsdp_descriptor {
    char signature[8];
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdt_address;

    // ACPI 2.0+ extended fields
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed));

namespace acpi
{
    class RSDP {
    private:
        static rsdp_descriptor* rsdp;
        /// @brief Checks if a RSDP is valid
        static bool is_valid_rsdp(const rsdp_descriptor* rsdp);
        /// @brief Scans memory for the RSDP
        /// @param start_phys Start PHYSICAL address of the region
        /// @param end_phys End PHYSICAL address of the region
        static rsdp_descriptor* scan_memory_for_rsdp(uintptr_t start_phys, uintptr_t end_phys);
    
    public:
        static rsdp_descriptor* find_rsdp(void* mb2_info);
    };
} // namespace acpi


#endif // RSDP_HPP
