// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// 8259A Programmable Interrupt Controller
// ========================================

#include <arch/interrupts/pic.hpp>
#include <graphics/kernel_gui.hpp>
#include <io.hpp>

using namespace arch;
using cpu::inb, cpu::outb, cpu::io_wait;

bool PIC_8259A::remaped = false;
bool PIC_8259A::disabled = false;

void PIC_8259A::remap(int offset1, int offset2) {
    // Save masks
    uint8_t a1 = inb(PIC_MASTER_DATA);
    uint8_t a2 = inb(PIC_SLAVE_DATA);
    
    outb(PIC_MASTER_COMMAND, ICW1_INIT | ICW1_ICW4); // Starts the init sequence (in cascade mode)
    io_wait();
	outb(PIC_SLAVE_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
    outb(PIC_MASTER_DATA, offset1); // ICW2: Master PIC vector offset
	io_wait();
	outb(PIC_SLAVE_DATA, offset2); // ICW2: Slave PIC vector offset
	io_wait();
	outb(PIC_MASTER_DATA, 1 << CASCADE_IRQ); // ICW3: tell Master PIC that there is a slave PIC at IRQ2
	io_wait();
	outb(PIC_SLAVE_DATA, 2); // ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_wait();

    outb(PIC_MASTER_DATA, ICW4_8086); // ICW4: have the PICs use 8086 mode (and not 8080 mode)
	io_wait();
	outb(PIC_SLAVE_DATA, ICW4_8086);
	io_wait();

    // Unmask both PICs.
	outb(PIC_MASTER_DATA, a1);
	outb(PIC_SLAVE_DATA, a2);

    remaped = true;
    kprintf(gui::PrintTypes::LOG_INFO, "Remapped the 8259A PIC\n");
}

void PIC_8259A::disable() {
    // Just masking everything to disable the PIC
    outb(PIC_MASTER_DATA, 0xff);
    outb(PIC_SLAVE_DATA, 0xff);
    disabled = true;
    kprintf(gui::LOG_INFO, "Disabled the 8259A PIC\n");
}

void PIC_8259A::mask_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if(irq < 8) {
        port = PIC_MASTER_DATA;
    } else {
        port = PIC_SLAVE_DATA;
        irq -= 8;
    }
    value = inb(port) | (1 << irq);
    outb(port, value);    
}

void PIC_8259A::unmask_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if(irq < 8) {
        port = PIC_MASTER_DATA;
    } else {
        port = PIC_SLAVE_DATA;
        irq -= 8;
    }
    value = inb(port) & ~(1 << irq);
    outb(port, value);  
}

void PIC_8259A::send_eoi(const uint8_t irq) {
    if(irq >= 8)
		outb(PIC_SLAVE_COMMAND, PIC_EOI);
	
	outb(PIC_MASTER_COMMAND, PIC_EOI);
}
