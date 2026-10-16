// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Task State Segment
// ========================================

#include <arch/tss.hpp>
#include <kernel_ui.hpp>
#include <mm/pmm.hpp>
#include <cpu.hpp>

using namespace arch;

void TSS::initialize(uint8_t cpu_id) {
    if (initialized) {
        kprintf(gui::PrintTypes::LOG_WARNING, "TSS already initialized on CPU #%u\n", cpu_id);
        return;
    }

    // Stacks

    void* double_fault_stack_bottom = mem::PMM::alloc_pages(DF_STACK_PAGES);
    if(double_fault_stack_bottom == nullptr) {
        kprintf(gui::PrintTypes::LOG_ERROR, "Couldn't allocate memory for Double Fault stack on CPU #%u!\n", cpu_id);
        return;
    }
    void* df_stack_top = (void*)((uint64_t)double_fault_stack_bottom + DF_STACK_PAGES * mem::PAGE_SIZE + mem::HHDM_BASE);
    
    void* nmi_stack_bottom = mem::PMM::alloc_pages(NMI_STACK_PAGES);
    if(nmi_stack_bottom == nullptr) {
        kprintf(gui::PrintTypes::LOG_ERROR, "Couldn't allocate memory for Non-Maskable Interrupt stack on CPU #%u!\n", cpu_id);
        return;
    }
    void* nmi_stack_top = (void*)((uint64_t)nmi_stack_bottom + NMI_STACK_PAGES * mem::PAGE_SIZE + mem::HHDM_BASE);

    void* mc_stack_bottom = mem::PMM::alloc_pages(MC_STACK_PAGES);
    if(mc_stack_bottom == nullptr) {
        kprintf(gui::PrintTypes::LOG_ERROR, "Couldn't allocate memory for Machine Check stack on CPU #%u!\n", cpu_id);
        return;
    }
    void* mc_stack_top = (void*)((uint64_t)mc_stack_bottom + MC_STACK_PAGES * mem::PAGE_SIZE + mem::HHDM_BASE);

    cpu::CPU* curr_cpu = cpu::CPU::get();
    // Kernel stack
    tss_entry.rsp0 = curr_cpu->kernel_stack;
    // Double Fault stack
    tss_entry.ist1 = reinterpret_cast<uint64_t>(df_stack_top);
    // Non-Maskable Interrupt stack
    tss_entry.ist2 = reinterpret_cast<uint64_t>(nmi_stack_top);
    // Machine Check
    tss_entry.ist3 = reinterpret_cast<uint64_t>(mc_stack_top);
    tss_entry.iopb = sizeof(tss_ent_t);

    tss_flush(0x28);

    if(!curr_cpu) {
        kprintf(gui::PrintTypes::LOG_ERROR, "Couldn't assign IST stacks to the CPU data on CPU #%u!\n", cpu_id);
        return;
    }

    curr_cpu->ist_stacks[0] = df_stack_top;
    curr_cpu->ist_stacks[1] = nmi_stack_top;
    curr_cpu->ist_stacks[2] = mc_stack_top;


    kprintf(gui::PrintTypes::LOG_INFO, "Initialized the TSS for ");
    if(cpu_id == 0) kprintf("the BSP\n");
    else kprintf("AP #%u\n", cpu_id);
    kprintf("   RSP0: 0x%x\n", tss_entry.rsp0);
    kprintf("   IST1: 0x%x\n", tss_entry.ist1);
    kprintf("   IST2: 0x%x\n", tss_entry.ist2);
    kprintf("   IST3: 0x%x\n", tss_entry.ist3);
    initialized = true;
}
