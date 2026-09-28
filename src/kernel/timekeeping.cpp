// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Kernel wall clock & monotonic timekeeping
// ========================================

#include <timekeeping.hpp>
#include <arch/rtc.hpp>
#include <kernel_ui.hpp>
#include <lib/string_util.hpp>

source_timer_t KernelTime::source_timer = {0, nullptr, nullptr};
volatile uint64_t KernelTime::boot_wall_ns = 0;
volatile uint64_t KernelTime::boot_ns = 0;

char KernelTime::signature[5] = "NULL";

static volatile uint64_t accumulated_monotonic_ns = 0;

void KernelTime::initialize(uint64_t src_hz, time_src_get_ns get_ns_funct, time_src_delay_us delay_us_funct, char signature[4]) {
    if(src_hz == 0 || !get_ns_funct || !delay_us_funct) {
        kprintf(gui::LOG_ERROR, "Tried initializing kernel timekeeping subsystem with invalid params!\n");
        return;
    }
    if (signature != nullptr) {
        strcpy(KernelTime::signature, signature);
        KernelTime::signature[4] = '\0';
    } else {
        strcpy(KernelTime::signature, "NULL");
        KernelTime::signature[4] = '\0';
    }

    // If upgrading from a fallback timer, save elapsed time so monotonic time doesn't reset to 0
    if (source_timer.get_ns_funct != nullptr) {
        accumulated_monotonic_ns += (source_timer.get_ns_funct() - boot_ns);
    } else {
        // First initialization during early boot
        boot_wall_ns = arch::RTC::get_unix_timestamp() * 1000000000ULL;
    }

    source_timer = {src_hz, get_ns_funct, delay_us_funct};
    
    // Baseline for the new timer
    boot_ns = get_ns_funct();

    kprintf(gui::LOG_INFO, "Initialized kernel timekeeping subsystem!\n");
}

uint64_t KernelTime::get_monotonic_ns() {
    if(!source_timer.get_ns_funct) return 0;

    uint64_t current_ns = source_timer.get_ns_funct();
    return accumulated_monotonic_ns + (current_ns - boot_ns);
}

uint64_t KernelTime::get_realtime_ns() {
    return boot_wall_ns + get_monotonic_ns();
}

void KernelTime::delay_us(uint64_t us) {
    if(source_timer.delay_us_funct) {
        source_timer.delay_us_funct(us);
    }
}
