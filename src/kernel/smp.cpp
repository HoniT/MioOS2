// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Symetric Multi-Processing
// ========================================

#include <smp.hpp>
#include <registry/system_topology_registry.hpp>
#include <cpu.hpp>
#include <util/atomic.hpp>
#include <lib/mem_util.hpp>
#include <kernel_panic.hpp>
#include <kernel_ui.hpp>

extern "C" uint8_t trampoline_start[];
extern "C" uint8_t trampoline_end[];
extern "C" uint32_t trampoline_data_32_addr;
extern "C" uint32_t trampoline_data_64_addr;
extern "C" uint32_t trampoline_data_phys_base;
extern "C" uint64_t trampoline_gdt_ptr;
extern "C" uint8_t trampoline32[];
extern "C" uint8_t trampoline64[];
extern "C" uint8_t trampoline_gdt[];

static uint64_t trampoline_phys_addr = 0;

static util::Atomic<uint32_t> ap_online_count = util::Atomic<uint32_t>(0);

void cpu::alloc_ap_cpus() {
    uint8_t ap_id_counter = 1;
    
    for (auto& ap_info : SystemTopology::cpu_infos) {
        if (ap_info.apic_id == SystemTopology::bsp_cpu.lapic_id)
            continue; // Skip the BSP

        cpu::CPU ap_node = CPU();
        ap_node.prepare_ap(ap_id_counter++, ap_info.apic_id);
        SystemTopology::ap_cpus.push_back(ap_node);
    }
}

bool cpu::ap_trampoline_setup(void* page) {
    if (!page) return false;
    
    trampoline_phys_addr = (uint64_t)page;
    
    uint64_t trampoline_virt_addr = trampoline_phys_addr + mem::HHDM_BASE;
    size_t trampoline_size = (size_t)(trampoline_end - trampoline_start);
    
    // Copy the trampoline
    memcpy((void*)trampoline_virt_addr, trampoline_start, trampoline_size);

    // Calculate patch offsets relative to the address
    uint64_t offset_32        = (uint64_t)&trampoline_data_32_addr - (uint64_t)trampoline_start;
    uint64_t offset_64        = (uint64_t)&trampoline_data_64_addr - (uint64_t)trampoline_start;
    uint64_t offset_phys_base = (uint64_t)&trampoline_data_phys_base - (uint64_t)trampoline_start;
    uint64_t offset_gdt_ptr   = (uint64_t)&trampoline_gdt_ptr - (uint64_t)trampoline_start;
    
    *(volatile uint32_t*)(trampoline_virt_addr + offset_32) = (uint32_t)(trampoline_phys_addr + ((uint64_t)&trampoline32 - (uint64_t)trampoline_start));
    *(volatile uint32_t*)(trampoline_virt_addr + offset_64) = (uint32_t)(trampoline_phys_addr + ((uint64_t)&trampoline64 - (uint64_t)trampoline_start));
    *(volatile uint32_t*)(trampoline_virt_addr + offset_phys_base) = (uint32_t)trampoline_phys_addr;
    
    // The GDT pointer requires the absolute address of the GDT itself
    *(volatile uint64_t*)(trampoline_virt_addr + offset_gdt_ptr + 2) = trampoline_phys_addr + ((uint64_t)&trampoline_gdt - (uint64_t)trampoline_start);
    
    kprintf(gui::LOG_INFO, "Set up AP trampoline at 0x%x\n", trampoline_phys_addr);
    return true;
}
