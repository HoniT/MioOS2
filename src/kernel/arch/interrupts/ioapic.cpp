// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// I/O Advanced Programmable Interrupt Controller
// ========================================

#include <arch/interrupts/ioapic.hpp>
#include <registry/system_topology_registry.hpp>
#include <mm/paging.hpp>
#include <kernel_ui.hpp>
#include <kernel_panic.hpp>

using namespace arch;

uint32_t IOAPIC::read_reg(uint32_t reg) {
    ioapic_virt_base[IOREGSEL / 4] = reg;
    return ioapic_virt_base[IOWIN / 4];
}

void IOAPIC::write_reg(uint32_t reg, uint32_t value) {
    ioapic_virt_base[IOREGSEL / 4] = reg;
    ioapic_virt_base[IOWIN / 4] = value;
}

void IOAPIC::write_rte(uint8_t gsi, uint8_t vector, uint8_t dest, uint16_t flags, bool masked, IRQDeliveryMode delv_mode, IRQDestinationMode dest_mode) {
    uint32_t low_index = IOREDTBL + (gsi * 2);
    uint32_t high_index = low_index + 1;

    rte_t rte {0};
    rte.vector = vector;
    rte.delvMode = delv_mode;
    rte.destMode = dest_mode;

    if(flags & 2) rte.pinPolarity = 1;
    if(flags & 8) rte.triggerMode = 1;
    if(masked) rte.mask = 1;

    rte.destination = dest; // Which LAPIC (or bitmask for logical, but I wont use that)

    write_reg(low_index, rte.lowerDword);
    write_reg(high_index, rte.upperDword);
}

uint32_t IOAPIC::get_gsi_for_irq(uint8_t irq, uint16_t& out_flags) {
    out_flags = 0; // Default flags
            
    for (auto& override : SystemTopology::overrides) {
        if (override.irq_source == irq) {
            out_flags = override.flags;
            return override.gsi;
        }
    }
    return irq; // If no override, IRQ == GSI (1:1 mapping)
}

bool IOAPIC::initialize() {
    // Mapping the I/O APIC
    mem::VirtAddr virt_addr = ioapic_info.ioapic_address + mem::HHDM_BASE;
    mem::PagingError err = mem::PagingBackend::map_page(virt_addr, ioapic_info.ioapic_address, mem::PageFlags::MMIO | mem::PageFlags::WriteThrough);
    if(err != mem::PagingError::Success) {
        kprintf(gui::LOG_ERROR, "Failed to map I/O APIC base with paging error %u\n", err);
        kernel_panic("Failed to map I/O APIC base\n");
        return false;
    }
    ioapic_virt_base = (uint32_t*)virt_addr;

    uint32_t ver_reg = read_reg(IOAPICVER);
    uint32_t max_entries = ((ver_reg >> 16) & 0xFF) + 1;

    // Mask ALL interrupts by default so we don't get unexpected traps
    for (uint32_t i = 0; i < max_entries; i++)
        write_rte(i, 0, 0, 0, true);

    kprintf(gui::LOG_INFO, "I/O APIC #%u initialized\n", ioapic_info.ioapic_id);
    initialized = true;
    return true;
}
