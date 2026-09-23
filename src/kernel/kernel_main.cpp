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
#include <io.hpp>
#include <drivers/serial.hpp>
#include <drivers/framebuffer.hpp>
#include <registry/output_registry.hpp>
#include <registry/system_topology_registry.hpp>
#include <kernel_ui.hpp>
#include <mm/pmm.hpp>
#include <mm/paging.hpp>
#include <mm/slub.hpp>
#include <arch/gdt.hpp>
#include <arch/tss.hpp>
#include <arch/apic_timer.hpp>
#include <arch/hpet.hpp>
#include <arch/pit.hpp>
#include <arch/tsc.hpp>
#include <timekeeping.hpp>
#include <arch/interrupts/idt.hpp>
#include <arch/interrupts/pic.hpp>
#include <arch/interrupts/lapic.hpp>
#include <arch/fpu.hpp>
#include <syscalls/syscalls.hpp>
#include <arch/acpi/acpi.hpp>
#include <arch/acpi/rsdp.hpp>
#include <arch/acpi/madt.hpp>
#include <uacpi/uacpi.h>
#include <uacpi/utilities.h>
#include <uacpi/event.h>
#include <tests/mm/paging_tests.hpp>
#include <tests/mm/buddy_tests.hpp>
#include <tests/mm/slub_tests.hpp>

static uint8_t early_uacpi_buffer[4096];

static void uacpi_init() {
    uacpi_status ret = uacpi_initialize(0);
    if (ret != UACPI_STATUS_OK) kernel_panic("Couldn't initialize uACPI"); 
    ret = uacpi_namespace_load();
    if (ret != UACPI_STATUS_OK) kernel_panic("Couldn't initialize uACPI"); 
    ret = uacpi_namespace_initialize();
    if (ret != UACPI_STATUS_OK) kernel_panic("Couldn't initialize uACPI"); 

    uacpi_set_interrupt_model(UACPI_INTERRUPT_MODEL_IOAPIC);
    ret = uacpi_finalize_gpe_initialization();
    if (uacpi_unlikely_error(ret)) {
        kprintf("uACPI GPE initialization error: %s", uacpi_status_to_string(ret));
        kernel_panic("Couldn't initialize uACPI"); 
        return;
    }
}

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
    arch::FPU_X87::initialize();

    // Timekeeping
    arch::TSC::calibrate();
    timekeeping_init();
    
    // ACPI & Interrupt controllers
    acpi::SystemDescriptionPointer::find_sdp(mbi);
    uacpi_status ret = uacpi_setup_early_table_access(early_uacpi_buffer, sizeof(early_uacpi_buffer));
    if (uacpi_unlikely_error(ret)) {
        kernel_panic("Failed to setup early uAPCI table access\n");
    }
    
    acpi::MADT::parse_madt();
    // APIC init
    arch::PIC_8259A::disable();
    mem::VirtAddr lapic_virt = SystemTopology::local_apic_base_phys + mem::HHDM_BASE;
    mem::PagingBackend::unmap_page(lapic_virt); // Prevent already mapped error
    mem::PagingError err = mem::PagingBackend::map_page(lapic_virt, SystemTopology::local_apic_base_phys, mem::PageFlags::MMIO);
    if(err != mem::PagingError::Success) {
        kprintf(gui::LOG_ERROR, "Failed to map Local APIC base with paging error %u\n", err);
        kernel_panic("Failed to map Local APIC base\n");
    }
    arch::LAPIC::initialize((uint32_t*)lapic_virt);
    // Initializing all the I/O APICs
    cpu::outb(0x22, 0x70); // IMCR
    cpu::outb(0x23, 0x01); // IMCR, switching to IOAPIC
    for(ioapic_info_t ioapic : SystemTopology::io_apics) {
        arch::IOAPIC ioapic_obj = arch::IOAPIC(ioapic);
        ioapic_obj.initialize();
        SystemTopology::io_apic_objs.push_back(ioapic_obj);
    }
    
    cpu::CPU::enable_interrupts();
    uacpi_init();

    // Other timers
    arch::APICTimer::initialize();
    if(arch::HPET::initialize())
        arch::HPET::setup_system_timer();

#ifdef DEBUG_BUILD_WARNING
    kprintf(RGB_COLOR_DARK_GRAY, "PS: All of the different subsystems log/print sensitive data about the machine \
(memory maps, addresses of vital hardware & software structures...). This is for development/debug purposes and is intentional! \
The sensitive data will be stripped away for a production/finished release if that day will come.\n");
#endif // DEBUG_BUILD_WARNING

    cpu::CPU::haltloop();
}
