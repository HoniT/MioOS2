// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef POWER_HPP
#define POWER_HPP

#include <stdint.h>
#include <uacpi/uacpi.h>

namespace pwr {
    class PowerManager {
    private:
        static uacpi_interrupt_ret on_system_powerdown(uacpi_handle ctx);

    public:
        static void install_sci_handler();
    };  
}; // namespace pwr

#endif // POWER_HPP
