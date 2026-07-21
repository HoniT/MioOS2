// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef SERIAL_HPP
#define SERIAL_HPP

#include <drivers/driver.hpp>
#include <stdint.h>

enum ComPorts {
    COM1 = 0x3F8,
    COM2 = 0x2F8,
    COM3 = 0x3E8,
    COM4 = 0x2E8,
    COM5 = 0x5F8,
    COM6 = 0x4F8,
    COM7 = 0x5E8,
    COM8 = 0x4E8
};

class SerialPortDriver : public Driver {
private:
    uint16_t port_base;

    /// @brief Returns if the transmit buffer is empty
    int is_transmit_empty();
    /// @brief Returns if a serial signal is received
    int serial_received();
public:
    /// @brief Creates driver object
    /// @param port_base COM port to use
    constexpr explicit SerialPortDriver(ComPorts port_base) : port_base(port_base) { }

    /// @brief Initializes the serial port driver
    bool initialize() override;
    void start() override { }
    void stop() override { }

    /// @brief Writes a single character using the COM port
    void write(char c);
    /// @brief Writes a string using the COM port
    void write(const char* str);

    /// @brief Reads a single character using the COM port
    char read();
};

#endif // SERIAL_HPP
