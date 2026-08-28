// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Programmable Interval Timer
// ========================================

#include <arch/pit.hpp>
#include <arch/interrupts/idt.hpp>
#include <arch/interrupts/ioapic.hpp>
#include <kernel_ui.hpp>
#include <registry/system_topology_registry.hpp>
#include <cpu.hpp>
#include <io.hpp>

using namespace arch;

volatile uint64_t PIT::ticks = 0;

bool PIT::initialized = false;
        
void PIT::initialize() {
    // Installing the handler
    IDT::register_interrupt_handler(PIT_VECTOR, PIT::on_irq);
    bool rte_status = IOAPIC::find_gsi_and_write_rte(PIT_VECTOR);
    if(SystemTopology::has_madt && !rte_status) {
        // The IOAPIC should've wrote the rte, but if the system doesnt have a madt that means we'll still use the 8259A PIC
        kprintf(gui::LOG_ERROR, "Couldn't write a I/O APIC RTE for the PIT interrupt vector\n");
        return;
    }

    uint32_t divisor = PIT_FREQUENCY / 1000;

    cpu::outb(IO_PIT_CMD, 0x34);
    cpu::outb(IO_PIT_CH0, (uint8_t)(divisor & 0xFF));
    cpu::outb(IO_PIT_CH0, (uint8_t)((divisor >> 8) & 0xFF));

    initialized = true;
    kprintf(gui::LOG_INFO, "Initialized the Programmable Interval Timer\n");
}

void PIT::on_irq(interrupt_registers_t* regs) {
    ticks++;
}
