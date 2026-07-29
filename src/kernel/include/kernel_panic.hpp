// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef KERNEL_PANIC_HPP
#define KERNEL_PANIC_HPP

/// @brief Kernel panic for manual calling
/// @param origin Origin of the kernel panic (the subsystem that called it)
/// @param msg Message of the kernel panic
void kernel_panic(const char* msg);

#endif // KERNEL_PANIC_HPP
