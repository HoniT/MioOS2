// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// I8042 PS/2 Controller and PS/2 driver interface
// ========================================

#include <drivers/ps2/ps2.hpp>
#include <drivers/ps2/keyboard.hpp>
#include <arch/acpi/fadt.hpp>
#include <uacpi/utilities.h>
#include <uacpi/namespace.h>
#include <kernel_ui.hpp>
#include <io.hpp>

using cpu::inb;
using cpu::outb;

util::Spinlock PS2Controller::spinlock;

static uacpi_iteration_decision ps2_match_cb(void* user_ctx, uacpi_namespace_node* node, uacpi_u32) {
    bool* found = (bool*)user_ctx;
    *found = true;
    return UACPI_ITERATION_DECISION_BREAK; // Stop searching once matched
}

bool PS2Controller::is_ps2_present() {
    bool exists_8042 = (acpi::FADT::fadt->iapc_boot_arch & (1 << 1)) != 0;
    if(exists_8042) return true;

    bool found = false;

    // Check for PS/2 Keyboard (PNP0303)
    uacpi_find_devices("PNP0303", ps2_match_cb, &found);
    if (found) return true;

    // Check for PS/2 Mouse (PNP0F13)
    uacpi_find_devices("PNP0F13", ps2_match_cb, &found);
    return found;
}

void PS2Controller::flush() {
    for (int i = 0; i < 64 && (inb(PS2_STATUS_PORT) & (1 << 0)); i++) {
        (void)inb(PS2_DATA_PORT); // Discarding data
        cpu::io_wait();
    }
}

bool PS2Controller::wait_read(uint32_t limit) {
    for (uint32_t i = 0; i < limit; i++) {
        if (inb(PS2_STATUS_PORT) & ST_OUT_FULL) return true;
        cpu::io_wait();
    }
    return false;
}

bool PS2Controller::wait_write(uint32_t limit) {
    for (uint32_t i = 0; i < limit; i++) {
        if (!(inb(PS2_STATUS_PORT) & ST_IN_FULL)) return true;
        cpu::io_wait();
    }
    return false;
}

bool PS2Controller::send_to_port(uint8_t port, uint8_t byte) {
    spinlock.lock();
    if (port == 2) {
        if (!wait_write()) {
            spinlock.unlock();
            return false;
        }
        outb(PS2_COMMAND_PORT, 0xD4);
    }
    if (!wait_write()) {
        spinlock.unlock();
        return false;
    }
    outb(PS2_DATA_PORT, byte);
    spinlock.unlock();
    return true;
}

uint8_t PS2Controller::read_data() {
    return inb(PS2_DATA_PORT);
}

bool PS2Controller::self_test() {
    if (!wait_write()) return false;
    outb(PS2_COMMAND_PORT, 0x20);

    if (!wait_read()) return false;
    uint8_t saved_ccb = inb(PS2_DATA_PORT);

    // Issue Controller Self-Test (0xAA)
    if (!wait_write()) return false;
    outb(PS2_COMMAND_PORT, 0xAA);

    if (!wait_read()) return false;
    uint8_t test_result = inb(PS2_DATA_PORT);

    if (wait_write()) {
        outb(PS2_COMMAND_PORT, 0x60);
        if (wait_write()) {
            outb(PS2_DATA_PORT, saved_ccb);
        }
    }

    // Check if controller returned 0x55
    return (test_result == 0x55);
}

bool PS2Controller::detect_dual_channel() {
    if (!wait_write()) return false;
    outb(PS2_COMMAND_PORT, 0xA8);

    if (!wait_write()) return false;
    outb(PS2_COMMAND_PORT, 0x20);

    if (!wait_read()) return false;
    uint8_t ccb = inb(PS2_DATA_PORT);

    // If set, the second port clock is permanently disabled (single channel)
    bool is_dual_channel = !(ccb & (1 << 5));

    if (is_dual_channel) {
        if (!wait_write()) return false;
        outb(PS2_COMMAND_PORT, 0xA7);

        // Clear Bit 1 (disable Port 2 IRQ) and Bit 5 (enable Port 2 Clock)
        ccb &= ~((1 << 1) | (1 << 5));

        if (wait_write()) {
            outb(PS2_COMMAND_PORT, 0x60);
            if (wait_write()) {
                outb(PS2_DATA_PORT, ccb);
            }
        }
    }

    return is_dual_channel;
}

ps2_ports_status_t PS2Controller::test_ports() {
    ps2_ports_status_t status = { false, false };

    if (wait_write()) {
        outb(PS2_COMMAND_PORT, 0xAB);
        if (wait_read()) {
            uint8_t result = inb(PS2_DATA_PORT);
            status.port1_works = (result == 0x00);
        }
    }

    if (is_dual_channel) {
        if (wait_write()) {
            outb(PS2_COMMAND_PORT, 0xA9);
            if (wait_read()) {
                uint8_t result = inb(PS2_DATA_PORT);
                status.port2_works = (result == 0x00);
            }
        }
    }

    return status;
}

static bool ps2_send_to_port(uint8_t port, uint8_t byte) {
    if (port == 2) {
        if (!PS2Controller::wait_write()) return false;
        outb(PS2_COMMAND_PORT, 0xD4);
    }
    if (!PS2Controller::wait_write()) return false;
    outb(PS2_DATA_PORT, byte);
    return true;
}

ps2_device_type_t PS2Controller::reset_and_identify_device(uint8_t port) {
    flush();

    if (!ps2_send_to_port(port, 0xFF)) return PS2_DEV_NONE;

    if (!wait_read(5000) || inb(PS2_DATA_PORT) != 0xFA) {
        return PS2_DEV_NONE; // No response or NACK -> unpopulated port
    }

    // Expect Self-Test Pass (0xAA)
    if (!wait_read(spin_limit_long)) return PS2_DEV_NONE;
    uint8_t test_byte = inb(PS2_DATA_PORT);
    
    if (test_byte == 0xFC) {
        // Self-test failed -> ignore device
        return PS2_DEV_NONE; 
    } else if (test_byte != 0xAA) {
        return PS2_DEV_NONE;
    }

    uint8_t id1 = 0xFF;
    if (wait_read(1000)) {
        id1 = inb(PS2_DATA_PORT);
    } else {
        // Send explicit Identify command (0xF2)
        ps2_send_to_port(port, 0xF2);
        if (wait_read(5000) && inb(PS2_DATA_PORT) == 0xFA) {
            if (wait_read(5000)) id1 = inb(PS2_DATA_PORT);
        }
    }

    // Classify device by ID
    if (id1 == 0xAB) {
        if (wait_read(1000)) {
            (void)inb(PS2_DATA_PORT); // Consume second byte of multi-byte keyboard ID
        }
        return PS2_DEV_KEYBOARD;
    } else if (id1 == 0x00 || id1 == 0x03 || id1 == 0x04) {
        return PS2_DEV_MOUSE;
    } else if (id1 == 0x41 || id1 == 0x83) {
        return PS2_DEV_KEYBOARD;
    }

    return PS2_DEV_UNKNOWN;
}

void PS2Controller::init_bus(bool port1_ok, bool port2_ok) {
    if (port1_ok) {
        ps2_device_type_t dev1 = reset_and_identify_device(1);
        if (dev1 == PS2_DEV_KEYBOARD) {
            // Keyboard init
            KeyboardDriver::get_instance(1)->initialize();
        }
        else if (dev1 == PS2_DEV_MOUSE) {
            // Mous init
        }
    }

    if (port2_ok) {
        ps2_device_type_t dev2 = reset_and_identify_device(2);
        if (dev2 == PS2_DEV_MOUSE) {
            // Mouse init
        }
        else if (dev2 == PS2_DEV_KEYBOARD) {
            KeyboardDriver::get_instance(2)->initialize();
        }
    }
}

void PS2Controller::initialize() {
    // 1. Checking if the PS/2 controller exists
    bool is_present = is_ps2_present();
    if(!is_present) {
        kprintf(gui::LOG_WARNING, "No PS/2 controller found on machine. Not initializing\n");
        return;
    }

    // 2. Disabling the devices
    if (wait_write()) outb(PS2_COMMAND_PORT, 0xAD);
    if (wait_write()) outb(PS2_COMMAND_PORT, 0xA7);

    // 3. Flushing
    flush();

    // 4. Setting the Controller Configuration Byte
    if (wait_write()) {
        outb(PS2_COMMAND_PORT, 0x20);
        if (wait_read()) {
            uint8_t ccb = inb(PS2_DATA_PORT);
            ccb &= ~((1 << 0) | (1 << 1) | (1 << 6)); // Clear IRQs and translation during initial setup
            ccb &= ~(1 << 4);                         // Enable Port 1 clock
            if (wait_write()) {
                outb(PS2_COMMAND_PORT, 0x60);
                if (wait_write()) {
                    outb(PS2_DATA_PORT, ccb);
                }
            }
        }
    }

    // 5. Perform Controller Self Test
    bool passed_test = self_test();
    if(!passed_test) {
        kprintf(gui::LOG_ERROR, "PS/2 controller couldn't pass self test. Not initializing\n");
        return;
    }

    // 6. Determine If There Are 2 Channels
    is_dual_channel = detect_dual_channel();

    // 7. Testing ports
    ps2_ports_status_t port_status = test_ports();
    if(!port_status.port1_works && !port_status.port2_works) {
        kprintf(gui::LOG_ERROR, "PS/2 ports couldn't pass self test. Not initializing\n");
        return;
    }

    // 8. Enabling devices
    if (port_status.port1_works) {
        if (!wait_write()) return;
        outb(PS2_COMMAND_PORT, 0xAE);
    }

    if (port_status.port2_works) {
        if (!wait_write()) return;
        outb(PS2_COMMAND_PORT, 0xA8);
    }

    if (!wait_write()) return;
    outb(PS2_COMMAND_PORT, 0x20);

    if (!wait_read()) return;
    uint8_t ccb = inb(PS2_DATA_PORT);

    // Enable IRQs and ensure clock lines are active
    if (port_status.port1_works) {
        ccb |= (1 << 0);
        ccb &= ~(1 << 4);
    }

    if (port_status.port2_works) {
        ccb |= (1 << 1);
        ccb &= ~(1 << 5);
    }

    if (!wait_write()) return;
    outb(PS2_COMMAND_PORT, 0x60);

    if (!wait_write()) return;
    outb(PS2_DATA_PORT, ccb);

    kprintf(gui::LOG_INFO, "Initialized the PS/2 controller (Is dual channel: %u)\n", is_dual_channel);
    
    // 9. Resetting devices
    init_bus(port_status.port1_works, port_status.port2_works);

    initialized = true;
}


bool PS2DeviceDriver::send_command_with_ack(uint8_t cmd) {
    int retries = 3;
    while (retries-- > 0) {
        if (!PS2Controller::send_to_port(port, cmd)) continue;
        
        if (PS2Controller::wait_read()) {
            uint8_t response = PS2Controller::read_data();
            if (response == 0xFA) return true;      // ACK
            if (response != 0xFE) return false;     // Not Resend (0xFE), fail
        }
    }
    return false;
}

