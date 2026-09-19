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

bool ACPI::is_valid_sdt_ent(acpi_header_t* table) {
    if (!table) return false;
    
    uint8_t sum = 0;
    uint8_t* ptr = (uint8_t*)table;
    
    for (uint32_t i = 0; i < table->length; i++) {
        sum += ptr[i];
    }
    
    return (sum == 0);
}

void ACPI::parse_tables() {
    for(char* sig : needed_acpi_table_signatures) {
        acpi_header_t* table = (acpi_header_t*)SystemDescriptionPointer::find_table_by_signature(sig);
        if (!table || !is_valid_sdt_ent(table)) {
            // kprintf(gui::LOG_ERROR, "Corrput or missing ACPI table: %s\n", sig);
            continue;
        }

        cached_acpi_headers.push_front(table);
    }
}

acpi_header_t* ACPI::get_table_by_signature(char sig[4]) {
    // First searching the cache
    for(acpi_header_t* table : cached_acpi_headers) 
        if(table && strcmp(table->signature, sig) == 0) return table;
    
    // If we didn't find it above we'll fall back to RSDP/XSDP parsing
    return SystemDescriptionPointer::find_table_by_signature(sig);
}
