# Interrupt Management Subsystem in MioOS

## Overview
The interrupt subsystem is responsible for handling both CPU exceptions and hardware-generated interrupts. The kernel initializes these components in stages during boot, transitioning from legacy hardware (8259A PIC) to the modern Advanced Programmable Interrupt Controller (APIC) architecture.

## Interrupt Descriptor Table (IDT)
The IDT is configured to handle 256 interrupt vectors:
* **CPU Exceptions (0-31):** Handled by `cpu_irq_handler` Specific exceptions (e.g., Double Fault, NMI) utilize dedicated Interrupt Stack Tables (ISTs) to prevent stack overflow issues Trap gates (type 0xF) and distinct Privilege Levels (DPL) are assigned to exceptions like Breakpoint (#BP)
* **Hardware Interrupts (32-255):** Handled by `hw_irq_handler` These are dispatched to dynamically registered callback functions
* **Spurious Interrupts (255):** Configured with a dedicated `spurious_irq_handler` that acts as a no-op and intentionally does not send an End of Interrupt (EOI) signal.

## Legacy 8259A Programmable Interrupt Controller (PIC)
To prevent legacy IRQs from conflicting with protected-mode CPU exceptions, the primary and secondary PICs are remapped during early initialization
* The Master PIC is mapped to vector offsets starting at `0x20` (32)
* The Slave PIC is mapped to vector offsets starting at `0x28` (40)
* Once the system confirms APIC presence and successfully transitions, the legacy PIC is fully disabled by masking all of its IRQ lines (writing `0xFF` to data ports).

## Local APIC (LAPIC)
The Local APIC manages interrupts specific to individual CPU cores
* It is mapped into virtual memory using MMIO (Memory-Mapped I/O) with cache-disabled and write-through paging flags.
* Legacy interrupts (LINT0 and LINT1) are masked during setup
* Hardware interrupts dispatched through the LAPIC require an End of Interrupt (EOI) signal upon completion, handled automatically by `hw_irq_handler`

## I/O APIC
The I/O APIC replaces the legacy 8259A PIC for system-wide hardware interrupts.
* **Redirection Table Entries (RTE):** Interrupts are routed to specific CPUs by writing to the IOAPIC's RTEs (64-bit entries mapped across two 32-bit registers).
* **Global System Interrupts (GSI):** Legacy IRQs are resolved into GSIs by parsing ACPI MADT Interrupt Source Overrides. 
* By default, all I/O APIC entries are masked during initialization to prevent unexpected traps

## Interrupt Control Flow
1. **Trigger:** A hardware device or CPU exception triggers an interrupt vector.
2. **Assembly Stub:** An ISR macro (defined in `idt.asm`) pushes a dummy error code (if the CPU didn't push one) and the interrupt vector number, then disables interrupts via `cli`
3. **Common Stub:** Execution jumps to a common stub (`cpu_isr_common_stub` or `hw_isr_common_stub`) which saves all general-purpose registers (r15-r8, rdi, rsi, rbp, rbx, rdx, rcx, rax)
4. **C++ Handler:** The system calls the C++ backend (`cpu_irq_handler` or `hw_irq_handler`), passing the saved `interrupt_registers_t` structure. 
5. **Resolution:** If unhandled, it triggers a kernel panic or logs an error If handled, it processes the interrupt and issues an EOI via the LAPIC (or PIC if still active)
6. **Restore:** The assembly stub restores all registers in exact reverse order, cleans up the stack, and executes `iretq` to resume normal execution