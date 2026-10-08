// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Symetric Multi-Processing
// ========================================

#include <smp.hpp>
#include <registry/system_topology_registry.hpp>
#include <cpu.hpp>

void cpu::alloc_ap_cpus() {
    for(int i = 0; i < SystemTopology::cpu_infos.size(); i++)
        SystemTopology::cpus.push_back(cpu::CPU());
}
