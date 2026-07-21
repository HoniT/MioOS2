// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Kernel printing utility
// ========================================

#include <graphics/kprint.hpp>
#include <registry/output_registry.hpp>
#include <lib/string_util.hpp>

void klogf(char* origin, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buf[1024];
    sprintf(buf, fmt, args);

    // Log with serial
    OutputRegistry::get_serial_logger()->write(origin);
    OutputRegistry::get_serial_logger()->write(": ");
    OutputRegistry::get_serial_logger()->write(buf);
    
    // TODO: Print to screen

    va_end(args);
}

void kprintf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buf[1024];
    sprintf(buf, fmt, args);

    // Log with serial
    OutputRegistry::get_serial_logger()->write(buf);
    
    // TODO: Print to screen

    va_end(args);
}
