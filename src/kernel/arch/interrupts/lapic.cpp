// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Local Advanced Programmable Interrupt Controller
// ========================================

#include <arch/interrupts/lapic.hpp>
#include <cpu.hpp>
#include <graphics/kernel_gui.hpp>
#include <mm/paging.hpp>
#include <registry/system_topology_registry.hpp>

using namespace arch;

volatile uint32_t* LAPIC::lapic_virt_base = nullptr;

bool LAPIC::has_apic() {
    uint32_t eax, ebx, ecx, edx;
    cpu::CPU::cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    return edx & (1 << 9) != 0;
}

bool LAPIC::initialize(uint32_t* lapic_virt_base) {
    if(!has_apic()) return false;
    if(!lapic_virt_base) return false;
    LAPIC::lapic_virt_base = lapic_virt_base;

    // Enabling the APIC hardware
    uint64_t ia32_apic_base = cpu::CPU::read_msr(0x1B);
    ia32_apic_base |= (1 << 11);
    cpu::CPU::write_msr(0x1B, ia32_apic_base);

    // Clear the TPR
    write_reg(REG_TPR, 0);

    // Set the SIVR
    // Vector 0xFF (255) is the standard choice.
    write_reg(REG_SIVR, SIVR_ENABLE | 0xFF);

    // Acknowledge any outstanding interrupts just in case
    send_eoi();

    // Mask Legacy Interrupts. LINT0 is usually wired to the legacy 8259 PIC. Since we disabled the PIC, 
    // we should mask LINT0. LINT1 is usually the NMI, masking it during boot is safe.
    write_reg(REG_LVT_LINT0, LVT_MASKED);
    write_reg(REG_LVT_LINT1, LVT_MASKED);

    // Mask the Error and Timer vectors until we are ready to set them up
    write_reg(REG_LVT_ERROR, LVT_MASKED);
    write_reg(REG_LVT_TIMER, LVT_MASKED);

    // Read the ID just to confirm it's responding
    uint32_t apic_id = read_reg(REG_ID) >> 24;
    kprintf(gui::LOG_INFO, "Local APIC initialized for core %u\n", apic_id);
    return true;
}

void LAPIC::send_eoi() {
    // Writing a non-zero value may cause a #GP
    write_reg(REG_EOI, 0);
}
