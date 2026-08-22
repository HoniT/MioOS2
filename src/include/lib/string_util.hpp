// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef STRING_UTIL_HPP
#define STRING_UTIL_HPP

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

/// @brief Calculates the length of a null-terminated string
size_t strlen(const char* str);
/// @brief Compares two strings
/// @return 0 if they are identical
int strcmp(const char* s1, const char* s2);
/// @brief A safer version of strcmp that stops comparing 
///         after a specified number of characters
int strncmp(const char* s1, const char* s2, size_t count);
/// @brief Copies a null-terminated string from source to destination
/// @return Destination address
char* strcpy(char* dest, const char* src);
/// @brief Copies up to count characters from source to destination. If the source is shorter than count, 
///         the rest of the destination is padded with null bytes
/// @return Destination address
char* strncpy(char* dest, const char* src, size_t count);
/// @brief Concatenates (appends) the source string to the end of the destination string
/// @return Destination address
char* strcat(char* dest, const char* src);

/// @brief Universal unsigned converter
/// @return Converted string
char* utoa(uint64_t value, char* str, int base);
/// @brief Universal signed converter
/// @return Converted string
char* itoa(int64_t value, char* str, int base);

/// @brief Formats a string and stores it in the provided buffer
/// @param buffer The destination buffer (must be large enough, this functions is pre-keap!)
/// @param fmt The format string
/// @param args Variadic arguments
/// @return The number of characters written (excluding the null terminator)
int sprintf(char* buffer, const char* fmt, va_list args);

#endif // STRING_UTIL_HPP
