// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Multiple APIC Description Table methods
// ========================================

#include <arch/acpi/madt.hpp>
#include <arch/acpi/rsdp.hpp>
#include <kernel_ui.hpp>
#include <kernel_panic.hpp>
#include <mm/mm_defs.hpp>
#include <registry/system_topology_registry.hpp>
#include <cpu.hpp>

using namespace acpi;

madt_t* MADT::madt = nullptr;

bool MADT::parse_madt() {
    madt_t* madt = (madt_t*)RSDP::find_table_by_signature("APIC");
    if(!madt) {
        kernel_panic("No Multiple APIC Description Table found on the system!\n");
        return false;
    }
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
                auto* lapic = (madt_lapic_ent_t*)record;
                // Bit 0 = Enabled, Bit 1 = Online Capable (ACPI 5.0+)
                // If either is set, we can use this CPU.
                bool is_bsp = lapic->apic_id == cpu::CPU::get_bsp_cpu().local_apic_id;
                if ((lapic->flags & 1) || (lapic->flags & 2)) {
                    SystemTopology::cpus.push_back({
                        lapic->acpi_cpu_id, 
                        lapic->apic_id,
                        lapic->flags,
                        is_bsp
                    });
                }
                kprintf("   Local APIC #%u %s\n", lapic->apic_id, is_bsp ? "(BSP)" : "");
                break;
            }
            case 1: {
                auto* ioapic = (madt_ioapic_ent_t*)record;
                SystemTopology::io_apics.push_back({
                    ioapic->ioapic_id, 
                    ioapic->ioapic_address, 
                    ioapic->gsib
                });
                kprintf("   I/O APIC\n");
                break;
            }
            case 2: {
                auto* iso = (madt_ioapic_iso_ent_t*)record;
                SystemTopology::overrides.push_back({
                    iso->bus_source, 
                    iso->irq_source, 
                    iso->gsi, 
                    iso->flags
                });
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
                auto* override = (madt_lapic_addr_ent_t*)record;
                lapic_phys = override->address;
                kprintf("   Local APIC Address Override (0x%x)\n", lapic_phys);
                break;
            }
            case 9: {
                auto* lx2apic = (madt_lx2apic_ent_t*)record;
                // Bit 0 = Enabled, Bit 1 = Online Capable (ACPI 5.0+)
                // If either is set, we can use this CPU.
                if ((lx2apic->flags & 1) || (lx2apic->flags & 2)) {
                    SystemTopology::lx2apics.push_back({
                        lx2apic->lx2apic_id, 
                        lx2apic->flags,
                        lx2apic->acpi_id
                    });
                }
                kprintf("   Local x2APIC #%u\n", lx2apic->lx2apic_id);
                break;
            }
            default: {
                kprintf(gui::LOG_ERROR, "Invalid MADT Entry found! (Type: %u)\n", record->type);
                break;
            }

        }
        current_record += record->length;
    }

    SystemTopology::local_apic_base_phys = lapic_phys;
    kprintf(gui::LOG_INFO, "MADT Parsing Complete: %u CPUs, %u IOAPICs found, 0x%x LAPIC base.\n", 
            SystemTopology::cpus.size(), SystemTopology::io_apics.size(), lapic_phys);

    return true;
}
