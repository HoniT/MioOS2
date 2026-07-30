// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// A permanent Global Descriptor Table for the kernel
// ========================================

#include <arch/gdt.hpp>
#include <arch/tss.hpp>
#include <graphics/kernel_gui.hpp>
#include <lib/mem_util.hpp>

using namespace arch;

bool GDT::initialized = false;
gdt_seg_t GDT::segs[GDT_SEGMENT_QUANTITY];
gdtr_t GDT::gdtr;

void GDT::initialize() {
    gdtr.size = GDT_SEGMENT_QUANTITY * sizeof(gdt_seg_t) - 1;
    gdtr.base = (uint64_t)segs;
    memset(&segs, 0, sizeof(gdt_seg_t) * GDT_SEGMENT_QUANTITY);

    set_descriptor(&segs[0], 0x0, 0x0,     0x0,  0x0);
    set_descriptor(&segs[1], 0x0, 0xFFFFF, 0x9A, 0xA);
    set_descriptor(&segs[2], 0x0, 0xFFFFF, 0x92, 0xC);
    set_descriptor(&segs[3], 0x0, 0xFFFFF, 0xF2, 0xA);
    set_descriptor(&segs[4], 0x0, 0xFFFFF, 0xFA, 0xC);

    uint64_t tss_base = (uint64_t)&TSS::tss_entry;
    set_descriptor(&segs[5], (uint32_t)tss_base, sizeof(tss_ent_t) - 1, 0x89, 0x0);
    // Upper 8 bytes of TSS (Base high 32 bits)
    uint32_t* tss_upper_half = (uint32_t*)&segs[6];
    tss_upper_half[0] = (uint32_t)(tss_base >> 32);
    tss_upper_half[1] = 0; // Reserved

    gdt_flush(&gdtr);

    kprintf(gui::PrintTypes::LOG_INFO, "Initialized the GDT for the BSP\n");
    initialized = true;
}

void GDT::set_descriptor(gdt_seg_t* seg, const uint32_t base, const uint32_t limit, 
                        const uint8_t access, const uint8_t flags) {
    if(seg == nullptr) return;
                            
    seg->limit_lo = (uint16_t)limit;
    seg->limit_hi = (uint8_t)(limit >> 16);

    seg->base_lo = (uint16_t)base;
    seg->base_mid = (uint8_t)(base >> 16);
    seg->base_hi = (uint8_t)(base >> 24);

    seg->access = access;

    seg->flags = flags;
}
