// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Time Stamp Counter
// ========================================

#include <arch/tsc.hpp>
#include <arch/pit.hpp>
#include <cpu.hpp>
#include <kernel_ui.hpp>

using namespace arch;

uint64_t TSC::tsc_hz = 0;
bool TSC::calibrated = false;

void TSC::calibrate() {
    uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;

    // Method 1: CPUID leaf 0x15 gives the exact TSC/crystal ratio and crystal Hz
    cpu::CPU::cpuid(0x15, 0, &eax, &ebx, &ecx, &edx);
    // EAX = denominator, EBX = numerator, ECX = crystal Hz
    if (eax != 0 && ebx != 0 && ecx != 0) {
        tsc_hz = ((uint64_t)ecx * ebx) / eax;

        calibrated = true;
        kprintf(gui::LOG_INFO, "Calibrated the TSC using CPUID 15h at %u Hz\n", tsc_hz);
        return; 
    }

    // Method 2: Fallback to CPUID leaf 0x16 for the base frequency in MHz
    cpu::CPU::cpuid(0x16, 0, &eax, &ebx, &ecx, &edx);
    if (eax != 0) {
        tsc_hz = (uint64_t)(eax & 0xFFFF) * 1000000;

        calibrated = true;
        kprintf(gui::LOG_INFO, "Calibrated the TSC using CPUID 16h at %u Hz\n", tsc_hz);
        return;
    }

    // Last resort: calibrating with PIT
    uint64_t start_tsc = rdtsc();
    PIT::poll_10ms(); 
    uint64_t end_tsc = rdtsc();
    tsc_hz = (end_tsc - start_tsc) * 100;

    calibrated = true;
    kprintf(gui::LOG_INFO, "Calibrated the TSC using the PIT at %u Hz\n", tsc_hz);
}

uint64_t TSC::rdtsc() {
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

void TSC::delay_us(uint64_t microseconds) {
    if (tsc_hz == 0) return;

    uint64_t start = rdtsc();
    uint64_t ticks_to_wait = (tsc_hz * microseconds) / 1000000;
    
    while ((rdtsc() - start) < ticks_to_wait) {
        asm volatile("pause");
    }
}

uint64_t TSC::get_ns() {
    if (tsc_hz == 0) return 0;
    return (rdtsc() * 1000000000ULL) / tsc_hz;
}
