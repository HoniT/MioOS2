// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef CPU_HPP
#define CPU_HPP

namespace cpu
{
    class CPU {
    public:
        /// @brief Stops the cpu forever
        [[noreturn]] static void haltloop();
    
        /// @brief Enables interrupts
        static void enable_interrupts();
    
        /// @brief Disables interrupts
        static void disable_interrupts();
    };
} // namespace cpu


#endif // CPU_HPP
