// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Real Time Clock
// ========================================

#include <arch/rtc.hpp>
#include <io.hpp>

using namespace arch;

uint8_t RTC::read_reg(uint8_t reg) {
    cpu::outb(RTC_PORT_CMD, reg);
	return bcd_to_bin(cpu::inb(RTC_PORT_DATA));
}

uint8_t RTC::bcd_to_bin(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

uint64_t RTC::get_unix_timestamp() {
    // Reading the CMOS values
    uint8_t sec =   bcd_to_bin(read_reg(0x0));
    uint8_t min =   bcd_to_bin(read_reg(0x2));
    uint8_t hour =  bcd_to_bin(read_reg(0x4));
    uint8_t day =   bcd_to_bin(read_reg(0x7));
    uint8_t month = bcd_to_bin(read_reg(0x8));
    uint16_t year = bcd_to_bin(read_reg(0x9));

    // Handling century wrap (I'll probably do something to this in the year 2070 if im not dead by then :))
    if (year < 70) year += 2000;
    else year += 1900;

    // Days for each month (non-leap year)
    const uint16_t days_before_month[] = {
        0, 31, 59, 90, 120, 151,
        181, 212, 243, 273, 304, 334
    };

    // Check leap year
    bool is_leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));

    // Total days since 1970
    uint32_t days = (year - 1970) * 365 + ((year - 1969) / 4) - ((year - 1901) / 100) + ((year - 1601) / 400);
    days += days_before_month[month - 1];
    if (is_leap && month > 2) days++;

    days += day - 1;

    return ((days * 24 + hour) * 60 + min) * 60 + sec;
}
