// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef PS2_HPP
#define PS2_HPP

#include <drivers/driver.hpp>
#include <stdint.h>
#include <util/spinlock.hpp>

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_COMMAND_PORT 0x64

#define ST_OUT_FULL (1 << 0)
#define ST_IN_FULL (1 << 1)

constexpr uint32_t spin_limit = 2000;
constexpr uint32_t spin_limit_long = 200000;

typedef struct {
    bool port1_works;
    bool port2_works;
} ps2_ports_status_t;

typedef enum {
    PS2_DEV_NONE,
    PS2_DEV_KEYBOARD,
    PS2_DEV_MOUSE,
    PS2_DEV_UNKNOWN
} ps2_device_type_t;

class PS2Controller {
private:
    static bool is_ps2_present();
    static void flush();

    
    static bool self_test();
    static bool detect_dual_channel();
    static ps2_ports_status_t test_ports();
    
    static ps2_device_type_t reset_and_identify_device(uint8_t port);
    static void init_bus(bool port1_ok, bool port2_ok);

    static inline bool initialized = false;
    static inline bool is_dual_channel = false;

    static util::Spinlock spinlock;
    
public:
    static bool wait_read(uint32_t limit = spin_limit);
    static bool wait_write(uint32_t limit = spin_limit);

    static bool send_to_port(uint8_t port, uint8_t data);
    static uint8_t read_data();

    static void initialize();
    static inline bool is_initialized() { return initialized; }
};

class PS2DeviceDriver : public Driver {
protected:
    uint8_t port;
    
    /// Handles the standard ACK (0xFA) and Resend (0xFE) protocol
    bool send_command_with_ack(uint8_t cmd);
    
public:
    PS2DeviceDriver(uint8_t assigned_port) : port(assigned_port) {}
    virtual ~PS2DeviceDriver() = default;
    
    virtual void handle_interrupt() = 0;
};

#endif // PS2_HPP
