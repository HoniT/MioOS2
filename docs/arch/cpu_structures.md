# CPU Structures Setup in MioOS

This document outlines the core structural components used to initialize and manage the CPU state, memory segmentation, and basic hardware exception handling.

## Global Descriptor Table (GDT)
The Global Descriptor Table is responsible for setting up fundamental memory segmentation. 
* The GDT manages memory segmentation using the `gdt_seg_t` descriptor structure.
* It is configured with 7 discrete segments, which include KERNEL_CODE (0x8), KERNEL_DATA (0x10), USER_CODE (0x18), and USER_DATA (0x20).
* The `gdtr_t` structure is utilized to store the overall size and base address of the table before it is flushed to the CPU.

## Task State Segment (TSS)
The Task State Segment is essential for providing valid stack pointers when transitioning between privilege levels or handling fatal exceptions.
* Hardware task state and stack pointers are maintained within the `tss_ent_t` structure.
* Dedicated Interrupt Stack Tables (IST) are mapped for critical faults: Double Faults use IST1, Non-Maskable Interrupts use IST2, and Machine Checks use IST3.
* Each of these dedicated hardware exception stacks is allocated exactly 1 memory page in size.

## Interrupt Descriptor Table (IDT)
The Interrupt Descriptor Table maps hardware and software interrupts to their respective handler routines. 
* The IDT defines exactly 256 vector entries, mapped using the `idt_gate_desc_t` structure.
* When an exception occurs, the CPU's state is preserved in the `interrupt_registers_t` structure, tracking registers like `rip`, `cs`, and `rflags`.
* For more details about interrupts and interrupt handling see the [Interrupts Documentation](interrupts.md).

## Core CPU and FPU State
The kernel aggressively caches CPU functionality and manages advanced coprocessor features.
* The `FPU_X87` class handles standard x87 floating-point initialization.
* The FPU XSAVE area size is derived via CPUID; if unsupported, it safely falls back to 512 bytes for legacy FXSAVE operations.
* Advanced processor setup dynamically configures control registers (CR0, CR4) and defines the Page Attribute Table (PAT).
* A dedicated `cpu_local_data_t` structure is allocated late in the initialization process to track localized states, such as the kernel stack, user stack, and APIC ID.

## Syscall Handling
The kernel utilizes Model-Specific Registers (MSRs) and optimized assembly routines to transition between user space and kernel space efficiently.
* Syscall extensions are initialized by enabling the SCE bit (bit 0) in the EFER MSR at address 0xC0000080.
* The `STAR` MSR (0xC0000081) is configured using the `KERNEL_CODE` segment to establish target segment descriptors for transitions.
* The `LSTAR` MSR (0xC0000082) is loaded with the address of the `syscall_entry` assembly routine to act as the primary entry point.
* The `FMASK` MSR (0xC0000084) is configured to automatically mask specific RFLAGS bits when a syscall is triggered.
* Upon entry, the `syscall_entry` routine executes the `swapgs` instruction to change the `GS` base so it points to the kernel's `cpu_local_data_t` instead of the user's context.
* The routine builds the current register state on the kernel stack by pushing values such as the user `rsp`, `rflags`, `rip`, and caller/callee-saved registers.
* The requested syscall number, stored in the `rax` register, is validated against `__NR_syscall_max`. 
* If the syscall number exceeds the maximum allowed limit, the routine jumps to a bad state and returns the `-ENOSYS` (-38) error code.
* To maintain compatibility with the System V ABI, the fourth syscall argument is moved from the `r10` register into the `rcx` register prior to calling the respective C++ handler.
* Once the syscall completes, interrupts are disabled, the registers are popped from the stack, and the CPU returns to user mode via the `o64 sysret` instruction.
* More syscall related documentation will come once I actually implement it. Now is just basic setup so the CPU can have a near finished state.