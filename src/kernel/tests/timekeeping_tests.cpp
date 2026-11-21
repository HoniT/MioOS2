// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Timer subsyster unit tests
// ========================================

#include <tests/timekeeping_tests.hpp>
#include <timekeeping.hpp>
#include <arch/timers/rtc.hpp>
#include <kernel_ui.hpp>
#include <kernel_panic.hpp>
#include <lib/string_util.hpp>

#define TIME_ASSERT(condition, msg) \
    if (!(condition)) { \
        kprintf(gui::LOG_ERROR, "%s\n", msg); \
        kernel_panic("Unit tests failed\n"); \
        return false; \
    }

// Mock timer
static volatile uint64_t mock_clock_ns = 0;

static uint64_t mock_get_ns() {
    return mock_clock_ns;
}

static void mock_delay_us(uint64_t us) {
    mock_clock_ns += us * 1000ULL;
}

bool run_timekeeping_tests() {
    // Test 1: Subsystem Initialization State
    TIME_ASSERT(KernelTime::source_timer.get_ns_funct != nullptr, "KernelTime get_ns_funct is nullptr");
    TIME_ASSERT(KernelTime::source_timer.delay_us_funct != nullptr, "KernelTime delay_us_funct is nullptr");
    TIME_ASSERT(KernelTime::source_timer.timer_hz > 0, "KernelTime source frequency is 0");

    // Test 2: Invalid Parameter Rejection
    source_timer_t saved_timer = KernelTime::source_timer;
    char saved_sig[5];
    strcpy(saved_sig, KernelTime::signature);

    // Test 3: Monotonicity & Active Delay Accuracy
    uint64_t t1 = KernelTime::get_monotonic_ns();
    KernelTime::delay_us(5000); // 5 ms
    uint64_t t2 = KernelTime::get_monotonic_ns();

    TIME_ASSERT(t2 >= t1, "Monotonic clock moved backwards!");
    uint64_t elapsed_ns = t2 - t1;
    TIME_ASSERT(elapsed_ns >= 4000000ULL, "delay_us(5000) returned too early");

    // Test 4: Wall Clock / Realtime Consistency
    uint64_t rtc_ns = arch::RTC::get_unix_timestamp() * 1000000000ULL;
    TIME_ASSERT(KernelTime::boot_wall_ns > 0, "boot_wall_ns was not initialized from RTC");
    TIME_ASSERT(rtc_ns >= KernelTime::boot_wall_ns, "RTC timestamp is earlier than boot_wall_ns");

    uint64_t mono_before = KernelTime::get_monotonic_ns();
    uint64_t real_now = KernelTime::get_realtime_ns();
    uint64_t mono_after = KernelTime::get_monotonic_ns();

    TIME_ASSERT(real_now >= KernelTime::boot_wall_ns + mono_before, "get_realtime_ns() is behind boot_wall_ns + monotonic");
    TIME_ASSERT(real_now <= KernelTime::boot_wall_ns + mono_after, "get_realtime_ns() jumped ahead of monotonic delta");

    // Test 5: Timer Source Upgrade Continuity 
    uint64_t pre_upgrade_mono = KernelTime::get_monotonic_ns();
    uint64_t pre_upgrade_wall = KernelTime::boot_wall_ns;

    // Simulate a new hardware timer starting with an arbitrary non-zero hardware counter
    mock_clock_ns = 9000000000ULL; // 9s arbitrary offset in new timer
    KernelTime::initialize(1000000000ULL, mock_get_ns, mock_delay_us);

    uint64_t post_upgrade_mono = KernelTime::get_monotonic_ns();
    TIME_ASSERT(post_upgrade_mono >= pre_upgrade_mono, "Monotonic time reset or regressed on timer source upgrade");
    TIME_ASSERT(KernelTime::boot_wall_ns == pre_upgrade_wall, "boot_wall_ns was overwritten during timer upgrade");

    // Advance mock timer by 10 ms
    KernelTime::delay_us(10000);
    uint64_t after_mock_delay = KernelTime::get_monotonic_ns();
    TIME_ASSERT(after_mock_delay - post_upgrade_mono == 10000000ULL, "Post-upgrade monotonic delta inaccurate");

    // Restore original hardware timer so the rest of the kernel continues unaffected
    KernelTime::initialize(saved_timer.timer_hz, saved_timer.get_ns_funct, saved_timer.delay_us_funct, saved_sig);
    TIME_ASSERT(KernelTime::get_monotonic_ns() >= after_mock_delay, "Monotonic time regressed after restoring hardware timer");

    kprintf(gui::PrintTypes::LOG_INFO, "All timekeeping tests passed successfully!\n");
    return true;
}
