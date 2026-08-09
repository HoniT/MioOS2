// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Serial output using the COM port
// ========================================

#include <drivers/serial.hpp>
#include <graphics/kernel_gui.hpp>
#include <io.hpp>

bool SerialPortDriver::initialize() {
    cpu::outb(port_base + 1, 0x00);
    cpu::outb(port_base + 3, 0x80);
    cpu::outb(port_base + 0, 0x01);
    cpu::outb(port_base + 1, 0x00);
    cpu::outb(port_base + 3, 0x03);
    cpu::outb(port_base + 2, 0xC7);
    cpu::outb(port_base + 4, 0x0B);

    cpu::outb(port_base + 4, 0x1E);
    cpu::outb(port_base + 0, 0xAE);

    int timeout = 10000;
    while ((cpu::inb(port_base + 5) & 1) == 0 && --timeout > 0);

    if (cpu::inb(port_base + 0) != 0xAE) {
        return false; // Hardware check failed
    }

    cpu::outb(port_base + 4, 0x0F);

    initialized = true;
    kprintf(gui::PrintTypes::LOG_INFO, "Initialized serial output to 0x%x\n", port_base);
    return true;
}

int SerialPortDriver::is_transmit_empty() {
   return cpu::inb(port_base + 5) & 0x20;
}

void SerialPortDriver::write(char c) {
    if(!initialized) return;

    while (is_transmit_empty() == 0);

    cpu::outb(port_base, c);
}

void SerialPortDriver::write(const char* str) {
    if(!initialized) return;

    for (int i = 0; str[i] != '\0'; i++) {
        write(str[i]);
    }
}


int SerialPortDriver::serial_received() {
   return cpu::inb(port_base + 5) & 1;
}

char SerialPortDriver::read() {
    if(!initialized) return '!';

    while (serial_received() == 0);

    return cpu::inb(port_base);
}
