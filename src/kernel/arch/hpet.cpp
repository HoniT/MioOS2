// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// High Precision Event Timer
// ========================================

#include <arch/hpet.hpp>
#include <arch/pit.hpp>
#include <kernel_ui.hpp>
#include <mm/paging.hpp>
#include <registry/system_topology_registry.hpp>
#include <arch/acpi/acpi.hpp>
#include <cpu.hpp>
#include <uacpi/tables.h>

using namespace arch;

bool HPET::initialized = false;
uint8_t* HPET::hpet_virt_base = nullptr;
uint32_t HPET::hpet_frequency = 0;
uint16_t HPET::minimum_tick = 0;
uint32_t HPET::ioapic_mask = 0;

util::List<hpet_cache_t> HPET::hpet_caches = util::List<hpet_cache_t>();

uint64_t HPET::read_reg(uint64_t reg) {
    return *((volatile uint64_t*)(hpet_virt_base + reg));
}

void HPET::write_reg(uint64_t reg, uint64_t value) {
    *((volatile uint64_t*)(hpet_virt_base + reg)) = value;
}


void HPET::init_timer_n(uint8_t timer_n) {
    hpet_cache_t curr_timer_cache = {0};
    uint64_t timer_config = read_reg(HPET_TIMER_CONFIG(timer_n));
    curr_timer_cache.is_periodic_capable = (timer_config & TIMER_PERIODIC_CAP) != 0;

    // Extract allowed routing and clamp it against actual IOAPIC pins
    uint32_t raw_routing_mask = (timer_config >> TIMER_ROUTING_SHIFT) & 0xFFFFFFFF;
    curr_timer_cache.valid_irq_mask = raw_routing_mask & ioapic_mask;
    curr_timer_cache.allocated_irq = 0;
    
    // Ensure interrupt generation is disabled initially, we dont turn it on for now
    timer_config &= ~TIMER_INT_ENABLE;
    write_reg(HPET_TIMER_CONFIG(timer_n), timer_config);

    hpet_caches.push_back(curr_timer_cache);
}


bool HPET::initialize() {
    uacpi_table hpet_table;
    uacpi_status status = uacpi_table_find_by_signature("HPET", &hpet_table);
    if(status != UACPI_STATUS_OK || hpet_table.hdr == nullptr) {
        kprintf(gui::LOG_ERROR, "Couldn't find HPET. Falling back to other timers!\n");
        return false;
    }
    hpet_t* hpet = (hpet_t*)hpet_table.hdr;

    // Mapping the MMIO
    mem::VirtAddr hpet_mmio_virt_base = hpet->address.address + mem::HHDM_BASE;
    mem::PagingBackend::unmap_page(hpet_mmio_virt_base);
    mem::PagingError err = mem::PagingBackend::map_page(hpet_mmio_virt_base, hpet->address.address, mem::PageFlags::MMIO);
    if(err != mem::PagingError::Success) {
        kprintf(gui::LOG_ERROR, "Couldn't map HPET MMIO base. Falling back to other timers!\n");
        return false;
    }
    hpet_virt_base = (uint8_t*)hpet_mmio_virt_base;

    // Disabling before modifying just in case
    uint64_t gen_config = read_reg(HPET_GENERAL_CONFIG);
    write_reg(HPET_GENERAL_CONFIG, gen_config & ~ENABLE_CNF);

    uint32_t tick_period = (HPET::read_reg(HPET_GENERAL_CAPS) >> 32) & 0xFFFFFFFF;
    if(tick_period == 0 || tick_period > 0x5F5E100) {
        kprintf(gui::LOG_ERROR, "Found invalid tick period for HPET. Falling back to other timers!\n");
        return false;
    }
    hpet_frequency = FS_IN_SECOND / tick_period;
    HPET::minimum_tick = hpet->minimum_tick;

    uint32_t ioapic_max_pins = SystemTopology::max_ioapic_entry();
    // Clamp I/O APIC pins (max we can do is 32)
    if(ioapic_max_pins > 32) ioapic_max_pins = 32;
    ioapic_mask = (1ULL << ioapic_max_pins) - 1;

    for(int timer_n = 0; timer_n < hpet->comparator_count; timer_n++)
        init_timer_n(timer_n);

    // Resetting the counter
    write_reg(HPET_MAIN_COUNTER, 0);

    // Enabling the timer
    gen_config = read_reg(HPET_GENERAL_CONFIG);
    write_reg(HPET_GENERAL_CONFIG, gen_config | ENABLE_CNF);

    kprintf(gui::LOG_INFO, "Initialized the High Precision Event Timer (%uHz)\n", hpet_frequency);
    initialized = true;
    return true;
}


uint64_t HPET::get_ticks() {
    if (!initialized) return 0;
    return read_reg(HPET_MAIN_COUNTER);
}

void HPET::sleep_us(uint64_t microseconds) {
    if (!initialized) return;

    uint64_t target_ticks = get_ticks() + (microseconds * (hpet_frequency / MICROSCND_IN_SECOND));
    
    while (get_ticks() < target_ticks) {
        asm volatile("pause");
    }
}

void arch::HPET::timer0_handler(interrupt_registers_t* regs) {
    // ...
}

void HPET::setup_system_timer(uint32_t freq_hz) {
    if (!initialized || hpet_caches.size() == 0) return;

    // Inspect Timer 0's valid hardware routing mask
    uint32_t valid_mask = hpet_caches.front().valid_irq_mask;
    uint8_t chosen_gsi;
    bool using_legacy_route = false;

    // Check if we have a valid explicit route
    if (valid_mask != 0) {
        // Explicit routing is available
        chosen_gsi = __builtin_ctz(valid_mask);
    } else {
        // Explicit routing failed
        // Fall back to Legacy Replacement Route (Timer 0 -> GSI 2).
        kprintf(gui::LOG_INFO, "HPET: No valid explicit GSI found. Falling back to Legacy Routing (GSI 2)\n");
        chosen_gsi = 2;
        using_legacy_route = true;
        
        // Enable Legacy Replacement Route in the General Config Register
        uint64_t gen_config = read_reg(HPET_GENERAL_CONFIG);
        write_reg(HPET_GENERAL_CONFIG, gen_config | LEG_RT_CNF);
    }
    hpet_caches.front().allocated_irq = chosen_gsi;

    // Register interrupt handler & IOAPIC routing
    IDT::register_interrupt_handler(HPET_SYSTEM_TIMER_VECTOR, timer0_handler);
    // if (!IOAPIC::find_gsi_and_write_rte(HPET_SYSTEM_TIMER_VECTOR)) {
    //     kprintf(gui::LOG_ERROR, "Failed to resolve GSI and write RTE for HPET Timer 0!\n");
    //     return;
    // }
    IOAPIC* ioapic = IOAPIC::get_ioapic_for_gsi(chosen_gsi, SystemTopology::io_apic_objs);
    if(!ioapic) {
        kprintf(gui::LOG_ERROR, "Failed to resolve GSI and write RTE for HPET Timer 0!\n");
        return;
    }
    ioapic->write_rte(chosen_gsi, HPET_SYSTEM_TIMER_VECTOR, cpu::CPU::get_bsp_cpu().local_apic_id, 0, false);

    uint64_t timer_config = read_reg(HPET_TIMER_CONFIG(0));
    
    if (!using_legacy_route) {
        // Only set explicit routing bits if we are NOT using legacy mode
        timer_config &= ~(TIMER_ROUTING_MASK << TIMER_ROUTING_SHIFT);
        timer_config |= ((uint64_t)chosen_gsi & TIMER_ROUTING_MASK) << TIMER_ROUTING_SHIFT;
    }

    if (hpet_caches.front().is_periodic_capable) {
        timer_config |= TIMER_INT_ENABLE | TIMER_PERIODIC_CAP | TIMER_VAL_SET_CNF;
        write_reg(HPET_TIMER_CONFIG(0), timer_config);

        uint64_t period_ticks = hpet_frequency / freq_hz;
        
        // Write initial absolute value to comparator 0
        write_reg(HPET_TIMER_COMP(0), get_ticks() + period_ticks);
        // Write repeating delta value (accumulator)
        write_reg(HPET_TIMER_COMP(0), period_ticks);

        kprintf(gui::LOG_INFO, "HPET Timer 0 configured in Periodic Mode at %u Hz on GSI %u\n", freq_hz, chosen_gsi);
    } else {
        // Fallback option if periodic mode is unsupported by hardware
        timer_config |= TIMER_INT_ENABLE;
        timer_config &= ~TIMER_PERIODIC_CAP;
        write_reg(HPET_TIMER_CONFIG(0), timer_config);
        
        uint64_t period_ticks = hpet_frequency / freq_hz;
        write_reg(HPET_TIMER_COMP(0), get_ticks() + period_ticks);

        kprintf(gui::LOG_WARNING, "HPET Timer 0 does not support periodic mode. Configured as One-Shot.\n");
    }
}
