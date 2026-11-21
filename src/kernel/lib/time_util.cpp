// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Utility functions for working with time
// ========================================

#include <lib/time_util.hpp>

time_units get_time_units_from_ns(uint64_t ns) {
    constexpr uint64_t NS_PER_SEC  = 1'000'000'000ULL;
    constexpr uint64_t SEC_PER_MIN = 60ULL;
    constexpr uint64_t SEC_PER_HR  = 3'600ULL;

    const uint64_t total_seconds = ns / NS_PER_SEC;

    return time_units{
        .hours   = static_cast<uint32_t>(total_seconds / SEC_PER_HR),
        .minutes = static_cast<uint8_t>((total_seconds / SEC_PER_MIN) % SEC_PER_MIN),
        .seconds = static_cast<uint8_t>(total_seconds % SEC_PER_MIN)
    };
}