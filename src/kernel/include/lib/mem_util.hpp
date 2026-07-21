// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef MEM_UTIL_HPP
#define MEM_UTIL_HPP

#include <stddef.h>
#include <stdint.h>

/// @brief Fills a block of memory with a specific byte value
/// @return Destination address
void* memset(void* dest, uint8_t val, size_t count);
/// @brief Copies a block of memory from a source to a destination 
///         for a given number of bytes
/// @attention memcpy assumes the memory regions do not overlap.
/// @return Destination address
void* memcpy(void* dest, const void* src, size_t count);
/// @brief Copies a block of memory from source to destination safely, 
///         even if the memory regions overlap
/// @return Destination address
void* memmove(void* dest, const void* src, size_t count);
/// @brief Compares two blocks of memory byte-by-byte
/// @return 0 if they are identical, 
///         or the difference between the first non-matching bytes
int memcmp(const void* s1, const void* s2, size_t count);

/// @brief Aligns a value up by a given alignment
/// @return Aligned value
uint64_t align_up(uint64_t value, size_t alignment);
/// @brief Aligns a value down by a given alignment
/// @return Aligned value
uint64_t align_down(uint64_t value, size_t alignment);

#endif // MEM_UTIL_HPP
