// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Kernel wall clock & monotonic timekeeping
// ========================================

#include <timekeeping.hpp>
#include <arch/tsc.hpp>
#include <arch/rtc.hpp>

static volatile uint64_t system_tsc_hz = 0;
static volatile uint64_t boot_tsc = 0;
static volatile uint64_t boot_wall_ns = 0;

void timekeeping_init() {
    system_tsc_hz = arch::TSC::get_tsc_hz();
    boot_tsc = arch::TSC::rdtsc();
    boot_wall_ns = arch::RTC::get_unix_timestamp() * 1000000000ULL;
}

uint64_t get_monotonic_ns() {
    uint64_t delta = arch::TSC::rdtsc() - boot_tsc;
    return (delta * 1000000000ULL) / system_tsc_hz;
}

uint64_t get_realtime_ns() {
    return boot_wall_ns + get_monotonic_ns();
}
