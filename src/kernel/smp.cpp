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
#include <timekeeping.hpp>
#include <arch/interrupts/lapic.hpp>
#include <mm/paging.hpp>

extern "C" uint8_t trampoline_start[];
extern "C" uint8_t trampoline_end[];
extern "C" uint8_t trampoline32[];
extern "C" uint8_t trampoline64[];
extern "C" uint8_t trampoline_gdt[];

// 32/64-bit jump patches
extern "C" uint32_t trampoline_data_32_addr;
extern "C" uint32_t trampoline_data_64_addr;
extern "C" uint32_t trampoline_data_phys_base;
extern "C" uint64_t trampoline_gdt_ptr;

// AP-specific loop patches
extern "C" uint64_t trampoline_data_cr3;
extern "C" uint64_t trampoline_data_rsp;
extern "C" uint64_t trampoline_data_entry;
extern "C" uint64_t trampoline_data_cpu_ptr;

static uint64_t trampoline_phys_addr = 0;

static util::Atomic<uint32_t> ap_online_count = util::Atomic<uint32_t>(0);

void cpu::alloc_ap_cpus() {
    uint8_t ap_id_counter = 1;
    
    for (auto& ap_info : SystemTopology::cpu_infos) {
        if (ap_info.apic_id == SystemTopology::bsp_cpu.lapic_id)
            continue; // Skip the BSP

        void* raw_mem = kmalloc(sizeof(cpu::CPU));
        if (!raw_mem) kernel_panic("OOM: Failed to allocate AP CPU structure!\n");

        memset(raw_mem, 0, sizeof(cpu::CPU));

        cpu::CPU* ap_node = new (raw_mem) cpu::CPU();

        ap_node->prepare_ap(ap_id_counter++, ap_info.apic_id);
        SystemTopology::ap_cpus.push_back(ap_node);
    }
}

bool cpu::ap_trampoline_setup(void* page) {
    if (!page) {
        kprintf(gui::LOG_ERROR, "Couldn't allocate memory for AP trampoline!\n");
        return false;
    }

    trampoline_phys_addr = (uint64_t)page;
    uint64_t trampoline_virt_addr = trampoline_phys_addr;
    mem::PagingError err = mem::PagingBackend::map_page(trampoline_virt_addr, trampoline_phys_addr, mem::PageFlags::KernelRWX);
    if(err != mem::PagingError::Success) {
        kprintf(gui::LOG_ERROR, "Couldn't identity map AP trampoline address!\n");
        return false;
    }
    
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

extern "C" void ap_entry(cpu::CPU* cpu_obj) {
    cpu_obj->init_cpu(false);

    cpu_obj->gdt.initialize(cpu_obj->cpu_id);

    mem::initialize_slub_ap();

    cpu_obj->tss.initialize(cpu_obj->cpu_id);
    arch::IDT::load(cpu_obj->cpu_id);

    arch::LAPIC::write_reg(0xF0, 0x1FF);

    ap_online_count++;

    while (true) {
        asm volatile("sti; \
            hlt");
    }
}

void cpu::smp_start_aps() {
    uint64_t bsp_cr3;
    asm volatile("mov %%cr3, %0" : "=r"(bsp_cr3));

    uint64_t trampoline_virt_addr = trampoline_phys_addr + mem::HHDM_BASE;

    uint64_t offset_cr3     = (uint64_t)&trampoline_data_cr3 - (uint64_t)trampoline_start;
    uint64_t offset_rsp     = (uint64_t)&trampoline_data_rsp - (uint64_t)trampoline_start;
    uint64_t offset_entry   = (uint64_t)&trampoline_data_entry - (uint64_t)trampoline_start;
    uint64_t offset_cpu_ptr = (uint64_t)&trampoline_data_cpu_ptr - (uint64_t)trampoline_start;

    uint32_t sipi_vector = (trampoline_phys_addr >> 12) & 0xFF;
    
    // Track how many APs should be awake by the end of the current loop iteration
    uint32_t expected_aps = 0; 

    for (auto& ap : SystemTopology::ap_cpus) {
        // Patch the trampoline variables for this specific AP
        *(volatile uint64_t*)(trampoline_virt_addr + offset_cr3)     = bsp_cr3;
        *(volatile uint64_t*)(trampoline_virt_addr + offset_rsp)     = ap->kernel_stack;
        *(volatile uint64_t*)(trampoline_virt_addr + offset_entry)   = (uint64_t)&ap_entry;
        *(volatile uint64_t*)(trampoline_virt_addr + offset_cpu_ptr) = (uint64_t)ap;

        uint32_t target_icr = ((uint32_t)ap->lapic_id) << 24;

        // INIT IPI
        arch::LAPIC::write_reg(0x310, target_icr);
        arch::LAPIC::write_reg(0x300, 0x00004500);
        
        KernelTime::delay_us(10000); 

        // SIPI 1 
        arch::LAPIC::write_reg(0x310, target_icr);
        arch::LAPIC::write_reg(0x300, 0x00004600 | sipi_vector);

        KernelTime::delay_us(200);

        // SIPI 2
        arch::LAPIC::write_reg(0x310, target_icr);
        arch::LAPIC::write_reg(0x300, 0x00004600 | sipi_vector);

        // Not modifying the trampoline for the next AP until this AP confirms it has booted
        expected_aps++;
        while (ap_online_count.load() < expected_aps) {
            asm volatile("pause");
        }
    }

    kprintf(gui::LOG_INFO, "All %u Application Processors booted successfully.\n", expected_aps);
}
