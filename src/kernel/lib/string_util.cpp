// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Utility functions for working with legacy const char* strings
// ========================================

#include <lib/string_util.hpp>

size_t strlen(const char* str) {
    if(str == nullptr) return 0;

    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

int strcmp(const char* s1, const char* s2) {
    if(s1 == nullptr || s2 == nullptr) return 0;
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t count) {
    if(s1 == nullptr || s2 == nullptr) return 0;
    if (count == 0) return 0;
    
    while (count-- > 0 && *s1 && (*s1 == *s2)) {
        if (count == 0) return 0;
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char* strcpy(char* dest, const char* src) {
    if(dest == nullptr || src == nullptr) return nullptr;

    char* d = dest;
    while ((*d++ = *src++)) {
        // The loop condition handles the copy and stops after copying '\0'
    }
    return dest;
}

char* strncpy(char* dest, const char* src, size_t count) {
    if(dest == nullptr || src == nullptr) return nullptr;

    char* d = dest;
    while (count > 0 && *src) {
        *d++ = *src++;
        count--;
    }
    while (count > 0) {
        *d++ = '\0';
        count--;
    }
    return dest;
}

char* strcat(char* dest, const char* src) {
    if(dest == nullptr || src == nullptr) return nullptr;

    char* d = dest;
    // Find the end of the destination string
    while (*d) {
        d++;
    }
    // Append the source string
    while ((*d++ = *src++)) {
        ;
    }
    return dest;
}

char* utoa(uint64_t value, char* str, int base) {
    if(str == nullptr) return nullptr;

    if (base < 2 || base > 36) {
        str[0] = '\0';
        return str;
    }

    char* ptr = str;
    
    // Handle 0 explicitly
    if (value == 0) {
        *ptr++ = '0';
        *ptr = '\0';
        return str;
    }

    const char chars[] = "0123456789ABCDEF"; 
    
    // Extract digits (this generates the string backwards)
    while (value > 0) {
        *ptr++ = chars[value % base];
        value /= base;
    }
    
    *ptr = '\0'; // Null-terminate the string

    // Reverse the string
    char* start = str;
    char* end = ptr - 1;
    while (start < end) {
        char temp = *start;
        *start++ = *end;
        *end-- = temp;
    }

    return str;
}

char* itoa(int64_t value, char* str, int base) {
    if(str == nullptr) return nullptr;

    char* ptr = str;
    uint64_t uvalue;

    // Only add a negative sign for base 10. 
    if (base == 10 && value < 0) {
        *ptr++ = '-';
        uvalue = (uint64_t)(-value);
    } else {
        uvalue = (uint64_t)value;
    }

    // Defer the rest of the work to the unsigned function
    utoa(uvalue, ptr, base);
    
    return str;
}

int sprintf(char* buffer, const char* fmt, va_list args) {
    int buf_idx = 0;
    
    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] == '%' && fmt[i+1] != '\0') {
            i++;
            char tmp_buf[65];
            
            switch (fmt[i]) {
                case 'd':
                case 'i': {
                    int64_t val = va_arg(args, int64_t);
                    itoa(val, tmp_buf, 10);
                    for (int j = 0; tmp_buf[j] != '\0'; j++) {
                        buffer[buf_idx++] = tmp_buf[j];
                    }
                    break;
                }
                case 'u': {
                    uint64_t val = va_arg(args, uint64_t);
                    utoa(val, tmp_buf, 10);
                    for (int j = 0; tmp_buf[j] != '\0'; j++) {
                        buffer[buf_idx++] = tmp_buf[j];
                    }
                    break;
                }
                case 'x':
                case 'X': {
                    uint64_t val = va_arg(args, uint64_t);
                    utoa(val, tmp_buf, 16);
                    for (int j = 0; tmp_buf[j] != '\0'; j++) {
                        buffer[buf_idx++] = tmp_buf[j];
                    }
                    break;
                }
                case 's': {
                    const char* s = va_arg(args, const char*);
                    if (!s) s = "(null)";
                    for (int j = 0; s[j] != '\0'; j++) {
                        buffer[buf_idx++] = s[j];
                    }
                    break;
                }
                case 'c': {
                    // Chars are promoted to ints
                    char c = (char)va_arg(args, int);
                    buffer[buf_idx++] = c;
                    break;
                }
                case '%': {
                    buffer[buf_idx++] = '%';
                    break;
                }
                default: {
                    // Unknown specifier: print the % and the unknown character literally
                    buffer[buf_idx++] = '%';
                    buffer[buf_idx++] = fmt[i];
                    break;
                }
            }
        } else {
            // Standard character
            buffer[buf_idx++] = fmt[i];
        }
    }
    
    buffer[buf_idx] = '\0';
    
    return buf_idx;
}
