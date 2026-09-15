// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef RTC_HPP
#define RTC_HPP

#include <stdint.h>

#define RTC_PORT_CMD 0x70
#define RTC_PORT_DATA 0x71

namespace arch
{
    class RTC {
    private:
        static uint8_t read_reg(uint8_t reg);
        static uint8_t bcd_to_bin(uint8_t val);
    
    public:
        static uint64_t get_unix_timestamp();
    };
} // namespace arch


#endif // RTC_HPP
