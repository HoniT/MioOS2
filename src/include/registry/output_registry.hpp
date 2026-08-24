// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef OUTPUT_REGISTRY_HPP
#define OUTPUT_REGISTRY_HPP

#include <drivers/serial.hpp>
#include <drivers/framebuffer.hpp>
#include <util/list.hpp>

class OutputRegistry {
private:
    inline static SerialPortDriver* serial_logger = nullptr;
    inline static FramebufferDriver* fb = nullptr;
public:
    // Not to be used untill slub is initialized
    inline static util::List<FramebufferDriver*> secondary_fbs = util::List<FramebufferDriver*>();

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
