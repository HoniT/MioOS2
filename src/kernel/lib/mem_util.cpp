// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Utility functions for working with memory
// ========================================

#include <lib/mem_util.hpp>

void* memset(void* dest, uint8_t val, size_t count) {
    if(dest == nullptr) return nullptr;

    uint8_t* p = (uint8_t*)dest;
    while (count--) {
        *p++ = (uint8_t)val;
    }
    return dest;
}

void* memcpy(void* dest, const void* src, size_t count) {
    if(dest == nullptr || src == nullptr) return nullptr;

    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (count--) {
        *d++ = *s++;
    }
    return dest;
}

void* memmove(void* dest, const void* src, size_t count) {
    if(dest == nullptr || src == nullptr) return nullptr;

    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    
    // If destination is below source, we can safely copy forwards
    if (d < s) {
        while (count--) {
            *d++ = *s++;
        }
    } 
    // If destination is above source, we must copy backwards to avoid overwriting
    else if (d > s) {
        d += count;
        s += count;
        while (count--) {
            *--d = *--s;
        }
    }
    return dest;
}

int memcmp(const void* s1, const void* s2, size_t count) {
    if(s1 == nullptr || s2 == nullptr) return 0;

    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;
    
    while (count--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

uint64_t align_up(uint64_t value, size_t alignment) {
    return (value + (alignment - 1)) & ~(alignment - 1);
}

uint64_t align_down(uint64_t value, size_t alignment) {
    return value & ~(alignment - 1);
}
