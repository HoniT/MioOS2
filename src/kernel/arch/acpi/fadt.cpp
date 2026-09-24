// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Fixed ACPI Description Table methods
// ========================================

#include <arch/acpi/fadt.hpp>
#include <uacpi/tables.h>
#include <uacpi/acpi.h>
#include <kernel_panic.hpp>
#include <kernel_ui.hpp>

using namespace acpi;

uint16_t FADT::sci_irq = 0;

void FADT::find_sci_irq() {
    uacpi_table fadt_table;
    uacpi_status ret;

    ret = uacpi_table_find_by_signature("FACP", &fadt_table);
    if (uacpi_unlikely_error(ret)) {
        uacpi_table_unref(&fadt_table);
        kernel_panic("No Fixed ACPI Description Table found on the system!\n");
        return;
    }

    acpi_fadt* fadt = (acpi_fadt*)(&fadt_table)->hdr;
    
    uint16_t sci_irq = fadt->sci_int;
    uacpi_table_unref(&fadt_table);

    FADT::sci_irq = sci_irq;
    kprintf(gui::LOG_INFO, "Found SCI: %u IRQ\n", sci_irq);
}
