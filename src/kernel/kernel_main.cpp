// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Kernel entry point and initialization
// ========================================

#include <kernel_main.hpp>
#include <multiboot.hpp>
#include <kernel_panic.hpp>
#include <cpu.hpp>
#include <drivers/serial.hpp>
#include <drivers/framebuffer.hpp>
#include <registry/output_registry.hpp>
#include <graphics/kernel_gui.hpp>
#include <mm/pmm.hpp>
#include <mm/paging.hpp>
#include <tests/mm/paging_tests.hpp>
#include <tests/mm/buddy_tests.hpp>
#include <arch/gdt.hpp>
#include <arch/tss.hpp>
#include <arch/interrupts/idt.hpp>

extern "C" void kernel_main(void* mbi, uint32_t magic) {
    // Initializing COM serial output
    SerialPortDriver serial_logger(COM1);
    OutputRegistry::set_serial_logger(&serial_logger);
    serial_logger.initialize();

    // Checking GRUB magic
    if(magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        kernel_panic("Invalid Multiboot2 magic passed to kernel!\n");
    }

    // Caching needed CPU features
    cpu::CPU::init_cpu_cache();

    // Early memory manager init
    multiboot_tag_mmap* mmap = Multiboot2::get_mmap(mbi);
    mem::PMM::initialize_bump(mmap, mbi);
    mem::PagingBackend::initialize(mmap);
    mem::run_paging_tests();

    // Framebuffer & graphics
    FramebufferDriver fb(Multiboot2::get_framebuffer(mbi));
    OutputRegistry::set_framebuffer(&fb);
    fb.initialize();
    gui::KernelGUI::initialize();

    // Early x86_64 subsystems
    arch::GDT::initialize();
    arch::TSS::initialize();
    arch::IDT::early_initialize();

    // Main memory manager init
    mem::PMM::initialize_buddy();
    mem::run_buddy_tests();

    cpu::CPU::haltloop();
}
