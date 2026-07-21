// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef OUTPUT_REGISTRY_HPP
#define OUTPUT_REGISTRY_HPP

#include <drivers/serial.hpp>

class OutputRegistry {
private:
    inline static SerialPortDriver* serial_logger = nullptr;
public:
    static SerialPortDriver* get_serial_logger() {
        return serial_logger;
    }
    static void set_serial_logger(SerialPortDriver* _serial_logger) {
        serial_logger = _serial_logger;
    }
};

#endif // OUTPUT_REGISTRY_HPP
