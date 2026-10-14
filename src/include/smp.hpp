// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef SMP_HPP
#define SMP_HPP

namespace cpu
{
    void alloc_ap_cpus();
    /// @brief Sets up AP trampoline
    /// @param page Page of where to copy the AP trampoline code. Must be below 1 MiB!!
    /// @return Success of trampoline setup
    bool ap_trampoline_setup(void* page);
    void smp_start_aps();
} // namespace cpu


#endif // SMP_HPP
