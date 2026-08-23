// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Multiple APIC Description Table methods
// ========================================

#include <arch/acpi/madt.hpp>
#include <arch/acpi/rsdp.hpp>
#include <graphics/kernel_gui.hpp>
#include <kernel_panic.hpp>
#include <mm/mm_defs.hpp>

using namespace acpi;

madt_t* MADT::madt = nullptr;

bool MADT::parse_madt() {
    madt_t* madt = (madt_t*)RSDP::find_table_by_signature("APIC");
    if(!madt) return false;
    MADT::madt = madt;

    uint64_t lapic_phys = madt->lapic_address;
    uint8_t* current_record = (uint8_t*)madt->entries; 
    uint8_t* end_of_table = (uint8_t*)madt + madt->acpi_header.length;
    kprintf(gui::LOG_INFO, "Found MADT at 0x%x with the following entries:\n", madt);

    // Itterating through the MADT entries
    while (current_record < end_of_table) {
        madt_ent_t* record = (madt_ent_t*)current_record;

        switch (record->type) {
            case 0: {
                kprintf("   Local APIC #%u\n", ((madt_lapic_ent_t*)record)->apic_id);
                break;
            }
            case 1: {
                kprintf("   I/O APIC\n");
                break;
            }
            case 2: {
                kprintf("   I/O APIC Interrupt Source Override\n");
                break;
            }
            case 3: {
                kprintf("   I/O APIC Non-Maskable Interrupt\n");
                break;
            }
            case 4: {
                kprintf("   Local APIC Non-Maskable Interrupt\n");
                break;
            }
            case 5: {
                kprintf("   Local APIC Address Override\n");
                break;
            }
            case 9: {
                kprintf("   Local x2APIC\n");
                break;
            }
            default: {
                kprintf(gui::LOG_ERROR, "Invalid MADT Entry found! (Type: %u)\n", record->type);
                break;
            }
        }

        current_record += record->length;
    }

    return true;
}
