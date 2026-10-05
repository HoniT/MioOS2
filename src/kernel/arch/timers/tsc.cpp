// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Time Stamp Counter
// ========================================

#include <arch/timers/tsc.hpp>
#include <arch/timers/pit.hpp>
#include <arch/timers/hpet.hpp>
#include <cpu.hpp>
#include <io.hpp>
#include <kernel_ui.hpp>
#include <timekeeping.hpp>
#include <lib/math.hpp>

using namespace arch;

uint64_t TSC::tsc_hz = 0;
bool TSC::calibrated = false;

bool TSC::is_tsc_invariant() {
    if(cpu::bsp_cpu.cpuid_cache.max_ext_leaf < 0x80000007) return false;

    uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
    cpu::CPU::cpuid(0x80000007, 0, &eax, &ebx, &ecx, &edx);

    // (Invariant TSC available)
    return (edx & (1 << 8)) != 0;
}

void TSC::calibrate() {
    if(!is_tsc_invariant()) {
        kprintf(gui::LOG_WARNING, "TSC isn't invariant. Not initializing!\n");
        return;
    }
    cpu::CPU::disable_interrupts();

    // We found a valid TSC, so demoting the PIT
    arch::PIT::demote();

    uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;

    // Method 1: CPUID leaf 0x15 gives the exact TSC/crystal ratio and crystal Hz
    if(cpu::bsp_cpu.cpuid_cache.max_std_leaf >= 0x15) {
        cpu::CPU::cpuid(0x15, 0, &eax, &ebx, &ecx, &edx);
        // EAX = denominator, EBX = numerator, ECX = crystal Hz
        if (eax != 0 && ebx != 0 && ecx != 0) {
            tsc_hz = ((uint64_t)ecx * ebx) / eax;
            
            calibrated = true;
            KernelTime::initialize(tsc_hz, get_ns, delay_us, "_TSC");
            
            kprintf(gui::LOG_INFO, "Calibrated the TSC using CPUID 15h at %u Hz\n", tsc_hz);
            cpu::CPU::enable_interrupts();
            return; 
        }
    }
        
    // Method 2: Fallback to CPUID leaf 0x16 for the base frequency in MHz
    if(cpu::bsp_cpu.cpuid_cache.max_std_leaf >= 0x16) {
        cpu::CPU::cpuid(0x16, 0, &eax, &ebx, &ecx, &edx);
        if (eax != 0) {
            tsc_hz = (uint64_t)(eax & 0xFFFF) * 1000000;
            
            calibrated = true;
            KernelTime::initialize(tsc_hz, get_ns, delay_us, "_TSC");
            
            kprintf(gui::LOG_INFO, "Calibrated the TSC using CPUID 16h at %u Hz\n", tsc_hz);
            cpu::CPU::enable_interrupts();
            return;
        }
    }
        
    // Last resort: calibrating with the PIT (10ms window)
    arch::PIT::prepare_10ms();
    
    uint64_t start_tsc = rdtsc();
    arch::PIT::poll_10ms();
    uint64_t end_tsc = rdtsc();
    
    // 10ms window * 100 = 1 second
    tsc_hz = (end_tsc - start_tsc) * 100;

    calibrated = true;
    KernelTime::initialize(tsc_hz, get_ns, delay_us, "_TSC");
    
    kprintf(gui::LOG_INFO, "Calibrated the TSC using the PIT at %u Hz\n", tsc_hz);
    cpu::CPU::enable_interrupts();
}

uint64_t TSC::rdtsc() {
    uint32_t lo, hi;
    asm volatile("lfence;\
        rdtsc;\
        lfence" : "=a"(lo), "=d"(hi) :: "memory");
    return ((uint64_t)hi << 32) | lo;
}

void TSC::delay_us(uint64_t microseconds) {
    if (tsc_hz == 0 || !calibrated) return;

    uint64_t start = rdtsc();
    uint64_t ticks_to_wait = mul_div_u64(microseconds, tsc_hz, 1000000ULL);
    
    while ((rdtsc() - start) < ticks_to_wait) {
        asm volatile("pause");
    }
}

uint64_t TSC::get_ns() {
    if (tsc_hz == 0 || !calibrated) return 0;
    return mul_div_u64(rdtsc(), 1000000000ULL, tsc_hz);
}
