// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// General ACPI logic: table parsing, caching...
// ========================================

#include <arch/acpi/acpi.hpp>
#include <arch/acpi/rsdp.hpp>
#include <kernel_ui.hpp>
#include <lib/string_util.hpp>

using namespace acpi;

util::List<acpi_header_t*> ACPI::cached_acpi_headers = util::List<acpi_header_t*>();

void ACPI::parse_tables() {
    SystemDescriptionPointer::parse_sdt(cached_acpi_headers);
}

acpi_header_t* ACPI::get_table_by_signature(char sig[4]) {
    // First searching the cache
    for(acpi_header_t* table : cached_acpi_headers) 
        if(table && strncmp(table->signature, sig, 4) == 0) return table;
    
    return nullptr;
}
