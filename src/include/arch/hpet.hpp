// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef HPET_HPP
#define HPET_HPP

#include <arch/acpi/acpi.hpp>
#include <util/list.hpp>
#include <arch/interrupts/idt.hpp>

// Femtoseconds in second
#define FS_IN_SECOND 1'000'000'000'000'000ULL
// Microseconds in second
#define MICROSCND_IN_SECOND 1'000'000ULL

// Offsets
#define HPET_GENERAL_CAPS 0x0
#define HPET_GENERAL_CONFIG 0x10
#define HPET_MAIN_COUNTER 0xF0
#define HPET_TIMER_CONFIG(n) (0x100 + (0x20 * (n)))
#define HPET_TIMER_COMP(n)   (0x108 + (0x20 * (n)))

// Bit flags
#define ENABLE_CNF (1 << 0)
#define LEG_RT_CNF (1 << 1)
#define TIMER_INT_ENABLE (1 << 2)
#define TIMER_PERIODIC_CAP (1 << 4)
#define TIMER_VAL_SET_CNF   (1ULL << 5)
#define TIMER_ROUTING_SHIFT 9
#define TIMER_ROUTING_MASK  0x1FULL

// Interrupt vector configuration for the system timer
#define HPET_SYSTEM_TIMER_VECTOR 34


namespace arch
{
    struct hpet_address_structure_t
    {
        uint8_t address_space_id;    // 0 - system memory, 1 - system I/O
        uint8_t register_bit_width;
        uint8_t register_bit_offset;
        uint8_t reserved;
        uint64_t address;
    } __attribute__((packed));

    struct hpet_t : acpi::acpi_header_t
    {
        uint8_t hardware_rev_id;
        uint8_t comparator_count:5;
        uint8_t counter_size:1;
        uint8_t reserved:1;
        uint8_t legacy_replacement:1;
        uint16_t pci_vendor_id;
        hpet_address_structure_t address;
        uint8_t hpet_number;
        uint16_t minimum_tick;
        uint8_t page_protection;
    } __attribute__((packed));

    struct hpet_cache_t {
        bool is_periodic_capable;
        uint32_t valid_irq_mask;
        uint8_t allocated_irq;
    };


    class HPET {
    private:
        static uint8_t* hpet_virt_base;
        static uint32_t hpet_frequency; // In femtoseconds
        static uint16_t minimum_tick;
        static uint32_t ioapic_mask;

        static util::List<hpet_cache_t> hpet_caches;

        /// @brief Initializes a specfic timer for the HPET
        /// @param timer_n The number of the timer (0 to hpet_t::comparator_count)
        static void init_timer_n(uint8_t timer_n);

        static void timer0_handler(interrupt_registers_t* regs);

    public:
        static bool initialized;
        static bool initialize();

        static uint64_t read_reg(uint64_t reg);
        static void write_reg(uint64_t reg, uint64_t value);

        static uint64_t get_ticks();
        static void sleep_us(uint64_t microseconds);

        /// @brief Sets up timer 0
        static void setup_system_timer(uint32_t freq_hz = 1000U);
    };
} // namespace arch


#endif // HPET_HPP
