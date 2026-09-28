// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef MATH_HPP
#define MATH_HPP

#include <stdint.h>

/// @brief Absolute value of X
int abs(int x);

uint64_t mul_div_u64_raw(uint64_t a, uint64_t b, uint64_t c);
uint64_t mul_div_u64(uint64_t a, uint64_t b, uint64_t c);

#endif // MATH_HPP
