// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Kernel entry point and initialization
// ========================================

#include <kernel_main.hpp>
#include <boot/multiboot.hpp>
#include <kernel_panic.hpp>
#include <cpu.hpp>
#include <drivers/serial.hpp>
#include <drivers/framebuffer.hpp>
#include <registry/output_registry.hpp>
#include <registry/system_topology_registry.hpp>
#include <graphics/kernel_gui.hpp>
#include <mm/pmm.hpp>
#include <mm/paging.hpp>
#include <mm/slub.hpp>
#include <arch/gdt.hpp>
#include <arch/tss.hpp>
#include <arch/interrupts/idt.hpp>
#include <arch/interrupts/pic.hpp>
#include <arch/interrupts/lapic.hpp>
#include <arch/fpu.hpp>
#include <syscalls/syscalls.hpp>
#include <arch/acpi/rsdp.hpp>
#include <arch/acpi/madt.hpp>
#include <tests/mm/paging_tests.hpp>
#include <tests/mm/buddy_tests.hpp>
#include <tests/mm/slub_tests.hpp>

extern "C" void kernel_main(void* mbi, uint32_t magic) {
    // Initializing COM serial output
    SerialPortDriver serial_logger(COM1);
    OutputRegistry::set_serial_logger(&serial_logger);
    serial_logger.initialize();

    // Checking GRUB magic
    if(magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        kernel_panic("Invalid Multiboot2 magic passed to kernel!\n");
    }

    // CPU features
    cpu::CPU::init_cpu_features_cache();
    cpu::CPU::init_features();

    // Early memory manager init
    multiboot_tag* mmap = Multiboot2::get_mmap(mbi);
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
    arch::IDT::early_initialize();
    
    // Main memory manager init
    mem::PMM::initialize_buddy();
    mem::run_buddy_tests();
    mem::initialize_slub();
    mem::run_slub_tests();
    
    // Full CPU structures init
    arch::TSS::initialize();
    arch::IDT::initialize();
    cpu::CPU::late_init_features();
    arch::syscall_msr_init();
    arch::X87_FPU::initialize();

    // Interrupt controllers
    acpi::RSDP::find_rsdp(mbi);
    bool has_madt = acpi::MADT::parse_madt();
    if(has_madt) {
        // Disabling the legacy PIC
        arch::PIC_8259A::disable();

        // APIC init
        mem::VirtAddr lapic_virt = SystemTopology::local_apic_base_phys + mem::HHDM_BASE;
        mem::PagingError err = mem::PagingBackend::map_page(lapic_virt, SystemTopology::local_apic_base_phys, mem::PageFlags::MMIO | mem::PageFlags::WriteThrough);
        if(err != mem::PagingError::Success) {
            kprintf(gui::LOG_ERROR, "Failed to map Local APIC base with paging error %u\n", err);
            kernel_panic("Failed to map Local APIC base\n");
        }
        arch::LAPIC::initialize((uint32_t*)lapic_virt);

        // Initializing all the I/O APICs
        for(ioapic_info_t ioapic : SystemTopology::io_apics) {
            arch::IOAPIC ioapic_obj = arch::IOAPIC(ioapic);
            ioapic_obj.initialize();
            SystemTopology::io_apic_objs.push_back(ioapic_obj);
        }
    }

    cpu::CPU::haltloop();
}
