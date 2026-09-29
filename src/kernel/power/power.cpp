// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Main power manager for the kernel
// ========================================

#include <power/power.hpp>
#include <kernel_ui.hpp>
#include <uacpi/event.h>
#include <uacpi/sleep.h>
#include <timekeeping.hpp>

using namespace pwr;

uacpi_interrupt_ret PowerManager::on_system_powerdown(uacpi_handle ctx) {
    kprintf(RGB_COLOR_RED, "\n\n\nShutting down");
    KernelTime::delay_us(1000000);
    uacpi_enter_sleep_state_simple(UACPI_SLEEP_STATE_S5);

    return UACPI_INTERRUPT_HANDLED;
}

void PowerManager::install_sci_handler() {
    uacpi_install_fixed_event_handler(UACPI_FIXED_EVENT_POWER_BUTTON, on_system_powerdown, nullptr);
}
