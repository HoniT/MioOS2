# MioOS Pre-Kernel Architecture

## 1. Early Boot Sequence
The initialization sequence begins in `src/boot/boot.asm`, which establishes the foundational Multiboot2 header tags, specifically configuring the framebuffer and appending the mandatory end tag. During this stage, the `.boot.bss` section is allocated to reserve space for the initial paging structures (PML4, PDPT, and PD) alongside a 16 KiB stack for the kernel.

The primary early boot procedure executes the following critical operations:
* **State Preparation:** Disables interrupts to ensure a safe execution environment and loads the initial kernel stack.
* **Bootloader State Preservation:** Caches the Multiboot2 information structure pointer and magic number for subsequent kernel use.
* **Hardware Verification:** Validates standard and extended CPUID availability, essential CPU features, and 64-bit long mode compatibility.
* **Memory Management Initialization:** Configures the paging structures for a Higher Half Kernel (HHK) & HHDM design, temporarily utilizing a 1 GiB identity map for lower memory.
* **CPU Feature Enablement:** Enables Physical Address Extension (PAE), loads the PML4 base address into the appropriate control register, and activates long mode, memory paging, and Streaming SIMD Extensions (SSE).
* **GDT Configuration:** Loads a temporary Global Descriptor Table (GDT) immediately prior to the 64-bit architecture transition.

## 2. 64-Bit Transition and Kernel Handoff
Following successful hardware initialization, the system officially transitions into 64-bit mode. This handoff phase involves:
* Clearing legacy segment registers to maintain a clean processor state.
* Shifting the stack pointer by a virtual offset of `0xFFFFFFFF80000000` to properly align with the Higher Half Kernel memory layout.
* Transferring execution control to `kernel_main`, which assumes responsibility for all subsequent operating system initialization.

As a final architectural failsafe, the bootloader implements a terminal halt loop to catch and safely handle any unexpected returns from the kernel entry point.
