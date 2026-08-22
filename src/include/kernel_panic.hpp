// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef KERNEL_PANIC_HPP
#define KERNEL_PANIC_HPP

#include <arch/interrupts/idt.hpp>

/// @brief Kernel panic for manual calling
/// @param origin Origin of the kernel panic (the subsystem that called it)
/// @param msg Message of the kernel panic
void kernel_panic(const char* msg);
/// @brief Automatic kernel panic for IDT
/// @param msg Message/Exception name
/// @param regs CPU state
void kernel_panic(const char* msg, arch::interrupt_registers_t* regs);

#endif // KERNEL_PANIC_HPP
