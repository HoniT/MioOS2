// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef TIMEKEEPING_HPP
#define TIMEKEEPING_HPP

#include <stdint.h>

typedef uint64_t (*time_src_get_ns)();
typedef void (*time_src_delay_us)(uint64_t);

struct source_timer_t {
    volatile uint64_t timer_hz;
    volatile time_src_get_ns get_ns_funct;
    volatile time_src_delay_us delay_us_funct;
};

class KernelTime {
private:
    static source_timer_t source_timer;

    static volatile uint64_t boot_wall_ns;
    static volatile uint64_t boot_ns;

    static char signature[5];

public:
    /// @brief Initializes timekeeping using info from a timer
    /// @param src_hz Time source frequency in hertz
    /// @param get_ns_funct A function of the time source that return nanoseconds since timer init
    /// @param delay_us_funct A function of the time source that delays microseconds
    /// @param signature Optional signature of the timer source
    static void initialize(uint64_t src_hz, time_src_get_ns get_ns_funct, time_src_delay_us delay_us_funct, char signature[4] = nullptr);

    /// @brief Gets monotonic time in ns
    static uint64_t get_monotonic_ns();
    /// @brief Gets realtime in ns (ns since 1970)
    static uint64_t get_realtime_ns();

    /// @brief Delays a given number of microseconds using the current source timer
    static void delay_us(uint64_t us);

    static inline const char* get_signature() { return signature; }
};

#endif // TIMEKEEPING_HPP
