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

struct rsdp_descriptor; // Forward declaration (definition in rsdp.hpp)

struct sdt_header_t {
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

struct rsdt_t {
    sdt_header_t header;
    uint32_t pointers[];
} __attribute__((packed));

struct xsdt_t {
    sdt_header_t header;
    uint64_t pointers[];
} __attribute__((packed));

struct cpu_core_t {
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

#endif // ACPI_HPP
