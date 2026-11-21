// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef KERNEL_MAIN_HPP
#define KERNEL_MAIN_HPP

#define DEBUG_BUILD_WARNING

#define OS_NAME "MioOS x86_64"
#define KERNEL_VERSION "0.2.0-dev (" __DATE__ " " __TIME__ ")"

#include <stdint.h>

/// @brief Entry point of the kernel
/// @param mbi Multiboot2 info
/// @param magic GRUB magic number
extern "C" void kernel_main(void* mbi, uint32_t magic);

#endif // KERNEL_MAIN_HPP
