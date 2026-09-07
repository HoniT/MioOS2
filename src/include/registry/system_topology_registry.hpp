// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef SYS_TOPOLOGY_REGISTRY_HPP
#define SYS_TOPOLOGY_REGISTRY_HPP

#include <stdint.h>
#include <util/list.hpp>
#include <arch/acpi/acpi.hpp>
#include <arch/interrupts/ioapic.hpp>

class SystemTopology {
public:
    inline static util::List<cpu_core_t> cpus = util::List<cpu_core_t>();
    inline static util::List<ioapic_info_t> io_apics = util::List<ioapic_info_t>();
    inline static util::List<ioapic_iso_t> overrides = util::List<ioapic_iso_t>();
    inline static util::List<lx2apic_t> lx2apics = util::List<lx2apic_t>();
    inline static uint64_t local_apic_base_phys = 0;

    inline static util::List<arch::IOAPIC> io_apic_objs = util::List<arch::IOAPIC>();

    /// @brief Returns the max I/O APIC entry the system has to offer
    inline static uint32_t max_ioapic_entry() {
        uint32_t max = 0;
        for(arch::IOAPIC ioapic : io_apic_objs) 
            if(ioapic.get_max_entries() > max)
                max = ioapic.get_max_entries();
        return max;
    }
};

#endif // SYS_TOPOLOGY_REGISTRY_HPP
