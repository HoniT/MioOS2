// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef OUTPUT_REGISTRY_HPP
#define OUTPUT_REGISTRY_HPP

#include <drivers/serial.hpp>
#include <drivers/framebuffer.hpp>

class OutputRegistry {
private:
    inline static SerialPortDriver* serial_logger = nullptr;
    inline static FramebufferDriver* fb = nullptr; // Only one fb driver for now untill heap
public:
    static SerialPortDriver* get_serial_logger() {
        return serial_logger;
    }
    static void set_serial_logger(SerialPortDriver* _serial_logger) {
        serial_logger = _serial_logger;
    }

    static FramebufferDriver* get_framebuffer() {
        return fb;
    }
    static void set_framebuffer(FramebufferDriver* _fb) {
        fb = _fb;
    }
};

#endif // OUTPUT_REGISTRY_HPP
