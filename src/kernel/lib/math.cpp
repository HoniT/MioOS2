// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Math methods
// ========================================

#include <lib/math.hpp>

int abs(int x) {
    return (x >= 0) ? x : -x;
}

uint64_t mul_div_u64_raw(uint64_t a, uint64_t b, uint64_t c) {
    uint64_t quot, rem;
    asm("mulq %2\n\t"
        "divq %3"
        : "=a"(quot), "=&d"(rem)
        : "r"(b), "r"(c), "0"(a)
        : "cc");
    return quot;
}

uint64_t mul_div_u64(uint64_t a, uint64_t b, uint64_t c) {
    uint64_t q = a / c;
    uint64_t r = a % c;
    return q * b + mul_div_u64_raw(r, b, c);
}
