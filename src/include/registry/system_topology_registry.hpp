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

class SystemTopology {
public:
    inline static util::List<cpu_core_t> cpus = util::List<cpu_core_t>();
    inline static util::List<ioapic_info_t> io_apics = util::List<ioapic_info_t>();
    inline static util::List<ioapic_iso_t> overrides = util::List<ioapic_iso_t>();
    inline static util::List<lx2apic_t> lx2apics = util::List<lx2apic_t>();
    inline static uint64_t local_apic_base_phys = 0;
};

#endif // SYS_TOPOLOGY_REGISTRY_HPP
