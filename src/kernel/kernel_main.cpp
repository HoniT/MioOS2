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
#include <registry/output_registry.hpp>
#include <graphics/kprint.hpp>
#include <mm/pmm.hpp>
#include <mm/paging.hpp>

extern "C" void kernel_main(void* mbi, uint32_t magic) {
    // Initializing COM serial output
    SerialPortDriver serial_logger(COM1);
    OutputRegistry::set_serial_logger(&serial_logger);
    serial_logger.initialize();

    // Checking GRUB magic
    if(magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        kernel_panic("KernelMain", "Invalid Multiboot2 magic passed to kernel!\n");
    }

    // Caching needed CPU features
    cpu::CPU::init_cpu_cache();

    // Early memory manager init
    mem::PMM::initialize_bump(Multiboot2::get_mmap(mbi), mbi);
    mem::PagingBackend::initialize();
}
