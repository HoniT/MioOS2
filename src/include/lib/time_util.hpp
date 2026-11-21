// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef TIME_UTIL_HPP
#define TIME_UTIL_HPP

#include <stdint.h>

struct time_units {
    uint32_t hours;
    uint8_t minutes;
    uint8_t seconds;
};

time_units get_time_units_from_ns(uint64_t ns);

#endif // TIME_UTIL_HPP
