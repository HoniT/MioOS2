// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef TIMEKEEPING_HPP
#define TIMEKEEPING_HPP

#include <stdint.h>

void timekeeping_init();
uint64_t get_monotonic_ns();
uint64_t get_realtime_ns();

#endif // TIMEKEEPING_HPP
