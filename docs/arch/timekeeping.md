# MioOS Timekeeping & Timers

This document provides an overview of the kernel's timekeeping subsystem and the various hardware timers it supports. The subsystem is designed to seamlessly upgrade from legacy timers to high-precision timers during the boot process without losing track of elapsed time.

## 1. The Timekeeping Subsystem (`KernelTime`)

The `KernelTime` class is the central timekeeping authority in the kernel. It abstracts the underlying hardware timers so the rest of the kernel doesn't need to know which hardware timer is currently active.

### Core Functions
*   **`get_monotonic_ns()`**: Returns the number of nanoseconds elapsed since the system booted. This value is guaranteed to strictly increase and is unaffected by wall-clock changes.
*   **`get_realtime_ns()`**: Returns the current wall-clock time in nanoseconds since the UNIX epoch (Jan 1, 1970).
*   **`delay_us(uint64_t us)`**: Delays the CPU execution for a specified number of microseconds using the best available timer.

### Timer Upgrades
`KernelTime` supports hot-swapping the active hardware timer. When a more precise timer (like HPET or TSC) is initialized, it calls `KernelTime::initialize()` to replace the current time source. The subsystem automatically calculates and preserves the accumulated monotonic time so that timer upgrades are completely transparent.

---

## 2. Hardware Timers

The kernel interacts with several hardware timers, each serving a specific role during the boot process and system execution.

### Programmable Interval Timer (PIT)
*   **Role**: Early boot fallback timer & Calibration tool.
*   **Behavior**: At early boot, the legacy 8254 PIT is initialized as the main timer running at 1000 Hz on IRQ 0. 
*   **Demotion**: Once a higher-precision timer (like HPET or TSC) is discovered, the PIT is "demoted". Its interrupts are unregistered to reduce overhead.
*   **Calibration**: The PIT's Channel 2 is retained to provide strict 10ms polling windows (`prepare_10ms()`, `poll_10ms()`), which are used to accurately measure and calibrate the frequencies of the TSC and APIC timers.

### Time Stamp Counter (TSC)
*   **Role**: Extremely high-precision time source.
*   **Behavior**: The kernel checks if the CPU supports an *Invariant TSC* (which doesn't fluctuate with CPU clock speeds). 
*   **Calibration**: It attempts to discover the exact TSC frequency via CPUID leaf `0x15`, falling back to `0x16`. If neither is available, it manually calibrates the TSC using the PIT's 10ms window.
*   **Usage**: Once calibrated, the TSC registers itself with `KernelTime` to provide sub-microsecond accurate delays and nanosecond timekeeping.

### High Precision Event Timer (HPET)
*   **Role**: Modern system timer.
*   **Behavior**: Discovered via ACPI tables and memory-mapped. The kernel configures HPET Timer 0. 
*   **Routing**: It attempts to use explicit IOAPIC routing. If unsupported, it falls back to Legacy Replacement Routing (GSI 2). It prefers Periodic Mode but can fall back to One-Shot if required by the hardware.
*   **Usage**: If initialized successfully, the HPET overtakes the PIT as the system's main hardware timer.

### Local APIC Timer
*   **Role**: Per-CPU local timer.
*   **Behavior**: Calibrated against the PIT during initialization.
*   **Modes**: 
    *   **TSC-Deadline**: If supported by the CPU, it operates in TSC-Deadline mode, which uses the TSC to fire an interrupt at an exact CPU cycle.
    *   **Periodic**: If TSC-Deadline is unavailable, it falls back to a standard 1ms periodic tick.

### Real Time Clock (RTC)
*   **Role**: Boot-time wall clock seed.
*   **Behavior**: Read exactly once during early boot. It queries the CMOS to get the current date and time, converts it to a UNIX timestamp, and seeds `KernelTime::boot_wall_ns`. After this, the kernel relies purely on monotonic hardware timers to track wall-clock time.

---

## 3. Initialization Flow

The boot sequence for timers occurs in `kernel_main.cpp` in the following order:

1.  **`PIT::initialize()`**: Starts the PIT to guarantee a working delay/tick system.
2.  **`TSC::calibrate()`**: Attempts to calibrate the TSC. If successful, it overrides the PIT in `KernelTime`.
3.  **`acpi::*` & `APIC` Init**: Basic hardware mappings and interrupt controllers are set up.
4.  **`APICTimer::initialize()`**: Sets up the per-core APIC timer for potential scheduler use.
5.  **`HPET::initialize()`**: If HPET exists, it takes over system timer duties and explicitly demotes the PIT's interrupt handler.