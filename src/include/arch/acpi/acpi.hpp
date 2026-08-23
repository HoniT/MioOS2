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

#endif // ACPI_HPP
