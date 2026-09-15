# Kernel Timers, Clocks & Timekeeping in MioOS

# Overview

The kernel uses multiple time sources and timers to provide:
- a **monotonic** high-resolution clock (derived from the Time Stamp Counter, TSC),
- a **wall-clock / realtime** baseline seeded from the RTC,
- hardware timers for interrupts and scheduling (PIT, Local APIC timer, HPET),
- fallbacks and calibration paths to derive frequencies and convert ticks <-> time.

Initialization and conversions are performed at boot: the TSC is calibrated where possible, the wall clock is recorded from the RTC, and available hardware timers (APIC/HPET/PIT) are configured for system ticks or one-shot/deadline use.

---

# Time sources (what they are & how the kernel uses them)

## Time Stamp Counter (TSC)

- Purpose: high-resolution, CPU-provided cycle counter used for monotonic time and microsecond delays.

- Calibration strategy (in order of preference):
  1. CPUID leaf `0x15` — exact TSC/crystal ratio and crystal Hz  
  2. CPUID leaf `0x16` — base frequency in MHz  
  3. PIT-based measurement — fallback

- Important functions:
  - `TSC::calibrate()` — performs calibration
  - `TSC::rdtsc()` — reads raw counter
  - `TSC::delay_us()` — busy-wait delay
  - `TSC::get_ns()` — converts TSC -> nanoseconds

- Notes:
  - Requires `tsc_hz` to be initialized before use

---

## RTC (Real-Time Clock)

- Purpose: provides a wall-clock Unix timestamp

- Usage:
  - Only used at boot to initialize `boot_wall_ns`
  - Acts as the base for realtime clock

---

## Programmable Interval Timer (PIT)

- Purpose: legacy timer used for calibration and fallback timing

- Behavior:
  - `prepare_10ms()` configures a one-shot ~10ms interval
  - `poll_10ms()` busy-waits until completion

- Usage:
  - Used for:
    - TSC calibration fallback
    - APIC calibration

---

## Local APIC Timer (APIC Timer)

- Purpose: per-CPU interrupt timer for scheduling

- Features:
  - Supports **Periodic Mode**
  - Supports **TSC-Deadline Mode** (if CPU supports it)

- Calibration:
  - Measures:
    - APIC ticks over 10ms
    - TSC ticks over 10ms
  - Derives:
    - APIC frequency
    - TSC frequency

- Modes:

### TSC-Deadline Mode
- Programs MSR with future TSC value
- High precision
- Avoids drift

### Periodic Mode
- Fixed interval interrupts (e.g., 1ms tick)
- Uses APIC counter reload

- IRQ handler:
  - Increments `total_ticks`
  - Re-arms deadline if needed

---

## High Precision Event Timer (HPET)

- Purpose: modern hardware timer with high precision and multiple comparators

- Initialization:
  - Found via ACPI
  - MMIO region mapped into virtual memory
  - Frequency calculated from hardware tick period

- Features:
  - Multiple timers (comparators)
  - Periodic or one-shot mode
  - Interrupt routing via IOAPIC

- System timer setup:
  - Attempts to route interrupt via valid GSI
  - Falls back to legacy routing if necessary
  - Configures periodic mode if supported

- Runtime:
  - `get_ticks()` — read counter
  - `sleep_us()` — busy-wait delay

---

# Boot-time initialization & conversions

## Initialization sequence

1. Calibrate TSC
2. Read RTC time
3. Store:
   - `boot_tsc`
   - `boot_wall_ns`
4. Use TSC deltas for runtime calculations

---

## Time calculations

### Monotonic time
```
delta = rdtsc() - boot_tsc
monotonic_ns = (delta * 1e9) / tsc_hz
```

### Realtime (wall clock)
```
realtime_ns = boot_wall_ns + monotonic_ns
```

---

# APIs & primitives

## Core timekeeping
- `timekeeping_init()`
- `get_monotonic_ns()`
- `get_realtime_ns()`

## TSC
- `TSC::calibrate()`
- `TSC::rdtsc()`
- `TSC::delay_us()`
- `TSC::get_ns()`

## PIT
- `PIT::prepare_10ms()`
- `PIT::poll_10ms()`

## APIC Timer
- `APICTimer::initialize()`
- `APICTimer::calibrate()`
- `APICTimer::set_deadline_us()`
- `APICTimer::on_irq()`

## HPET
- `HPET::initialize()`
- `HPET::setup_system_timer(freq)`
- `HPET::get_ticks()`
- `HPET::sleep_us()`

---

# Design decisions & rationale

- **Multiple time sources**
  - Use best available hardware dynamically

- **Calibration-first design**
  - Prefer CPUID methods
  - Fall back to measured timers

- **Fallback safety**
  - PIT always available as last resort

- **Precision vs compatibility**
  - TSC for precision
  - HPET/APIC for interrupts

---

# Caveats & considerations

## TSC issues
- May not be synchronized across CPUs
- May vary with CPU frequency (unless invariant TSC)

## Integer math
- Uses 64-bit arithmetic to avoid overflow

## Busy waiting
- Used in:
  - PIT
  - HPET sleep
  - TSC delay
- Not suitable for long waits

## APIC deadline mode
- Relies on MSR writes
- Hardware-dependent

## HPET routing
- May fall back to legacy IRQ routing
- Depends on IOAPIC configuration

---

# Practical usage examples

## Initialization

```c
arch::TSC::calibrate();
timekeeping_init();
```

## Getting time

```c
uint64_t mono = get_monotonic_ns();
uint64_t real = get_realtime_ns();
```

## APIC timer

```c
APICTimer::initialize();
```

## HPET

```c
if (HPET::initialize()) {
    HPET::setup_system_timer(1000);
}
```


---

# Summary

The kernel time subsystem is built around:
- **TSC** -> high-resolution time
- **RTC** -> initial wall clock
- **APIC / HPET** -> interrupts and scheduling
- **PIT** -> fallback calibration

This layered design ensures:
- high precision where possible
- compatibility across hardware
- reliable fallback mechanisms

---