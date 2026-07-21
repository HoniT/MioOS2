// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef KPRINT_HPP
#define KPRINT_HPP

/// @brief Outputs a log: [Origin]: [Message]
/// @param origin Origin of the log (which subsystem it was called from, e.g. PMM, GDT, ...)
/// @param fmt Format of the message
/// @param ... va list of args
void klogf(char* origin, const char* fmt, ...);

/// @brief Basic printing
/// @param fmt Format of the message
/// @param ... va list of args
void kprintf(const char* fmt, ...);

#endif // KPRINT_HPP
