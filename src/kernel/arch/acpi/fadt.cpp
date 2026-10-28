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
acpi_fadt* FADT::fadt = nullptr;

void FADT::find_fadt() {
    uacpi_status status = uacpi_table_fadt(&fadt);
    if (uacpi_unlikely_error(status) || !fadt) {
        kernel_panic("No Fixed ACPI Description Table found on the system!\n");
        return;
    }
}

void FADT::find_sci_irq() {
    if(!fadt) return;
    
    sci_irq = fadt->sci_int;
    kprintf(gui::LOG_INFO, "Found SCI: %u IRQ\n", sci_irq);
}
