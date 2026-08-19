// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef SYSCALLS_HPP
#define SYSCALLS_HPP

#include <stdint.h>

namespace arch
{
    using syscall_fn_t = int64_t (*)(uint64_t arg1, uint64_t arg2, uint64_t arg3,
                                  uint64_t arg4, uint64_t arg5, uint64_t arg6);\

    /// @brief Initializes needed MSRs for syscalls
    void syscall_msr_init();

    extern "C" void syscall_entry();
    extern "C" const uint64_t __NR_syscall_max;
    extern "C" const syscall_fn_t syscall_table[];
} // namespace arch

#endif // SYSCALLS_HPP
