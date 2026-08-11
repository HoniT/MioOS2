// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Task State Segment
// ========================================

#include <arch/tss.hpp>
#include <graphics/kernel_gui.hpp>
#include <mm/pmm.hpp>

using namespace arch;

extern "C" uint8_t stack_top[];
void* df_stack_top;
void* nmi_stack_top;
void* mc_stack_top;

bool TSS::initialized = false;
tss_ent_t TSS::tss_entry{0};

void TSS::initialize() {
    if (initialized) return;

    // Stacks

    void* double_fault_stack_bottom = mem::PMM::alloc_pages(DF_STACK_PAGES);
    if(double_fault_stack_bottom == nullptr) {
        kprintf(gui::PrintTypes::LOG_ERROR, "Couldn't allocate memory for Double Fault stack!\n");
        return;
    }
    df_stack_top = (void*)((uint64_t)double_fault_stack_bottom + DF_STACK_PAGES * mem::PAGE_SIZE);
    
    void* nmi_stack_bottom = mem::PMM::alloc_pages(NMI_STACK_PAGES);
    if(nmi_stack_bottom == nullptr) {
        kprintf(gui::PrintTypes::LOG_ERROR, "Couldn't allocate memory for Non-Maskable Interrupt stack!\n");
        return;
    }
    nmi_stack_top = (void*)((uint64_t)nmi_stack_bottom + NMI_STACK_PAGES * mem::PAGE_SIZE);

    void* mc_stack_bottom = mem::PMM::alloc_pages(MC_STACK_PAGES);
    if(mc_stack_bottom == nullptr) {
        kprintf(gui::PrintTypes::LOG_ERROR, "Couldn't allocate memory for Machine Check stack!\n");
        return;
    }
    mc_stack_top = (void*)((uint64_t)mc_stack_bottom + MC_STACK_PAGES * mem::PAGE_SIZE);

    // Kernel stack
    tss_entry.rsp0 = reinterpret_cast<uint64_t>(stack_top) + mem::HHDM_BASE;
    // Double Fault stack
    tss_entry.ist1 = reinterpret_cast<uint64_t>(df_stack_top) + mem::HHDM_BASE;
    // Non-Maskable Interrupt stack
    tss_entry.ist2 = reinterpret_cast<uint64_t>(nmi_stack_top) + mem::HHDM_BASE;
    // Machine Check
    tss_entry.ist3 = reinterpret_cast<uint64_t>(mc_stack_top) + mem::HHDM_BASE;
    tss_entry.iopb = sizeof(tss_ent_t);

    tss_flush(0x28);

    kprintf(gui::PrintTypes::LOG_INFO, "Initialized the TSS for the BSP\n");
    kprintf("   RSP0: 0x%x\n", tss_entry.rsp0);
    kprintf("   IST1: 0x%x\n", tss_entry.ist1);
    kprintf("   IST2: 0x%x\n", tss_entry.ist2);
    kprintf("   IST3: 0x%x\n", tss_entry.ist3);
    initialized = true;
}
