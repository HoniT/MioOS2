// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// OS Services Layer for uACPI (A lot are just studs)
// ========================================

#include <uacpi/kernel_api.h>
#include <arch/acpi/rsdp.hpp>
#include <mm/slub.hpp>
#include <mm/paging.hpp>
#include <io.hpp>
#include <cpu.hpp>
#include <kernel_ui.hpp>
#include <timekeeping.hpp>
#include <arch/interrupts/idt.hpp>
#include <arch/interrupts/ioapic.hpp>
#include <registry/system_topology_registry.hpp>

namespace {
    struct UacpiHandlerEntry {
        uacpi_interrupt_handler handler = nullptr;
        uacpi_handle ctx = nullptr;
        bool in_use = false;
        uint8_t vector = 0;
    };

    constexpr size_t UACPI_MAX_HANDLERS = 8;
    UacpiHandlerEntry g_uacpi_handlers[UACPI_MAX_HANDLERS];

    UacpiHandlerEntry* allocate_uacpi_entry(uint8_t vector) {
        for (auto& entry : g_uacpi_handlers) {
            if (!entry.in_use) {
                entry.in_use = true;
                entry.vector = vector;
                return &entry;
            }
        }
        return nullptr;
    }

    UacpiHandlerEntry* get_uacpi_entry(uint8_t vector) {
        for (auto& entry : g_uacpi_handlers) {
            if (entry.in_use && entry.vector == vector)
                return &entry;
        }
        return nullptr;
    }

    void uacpi_interrupt_trampoline(arch::interrupt_registers_t* regs) {
        UacpiHandlerEntry* entry = get_uacpi_entry(static_cast<uint8_t>(regs->interr_no));
        if (entry && entry->handler)
            entry->handler(entry->ctx);
    }
}

extern "C" {
    uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rsdp_address) {
        void* sdp = acpi::SystemDescriptionPointer::get_sdp_ptr();
        if(!sdp) return UACPI_STATUS_NOT_FOUND;

        uintptr_t sdp_addr = reinterpret_cast<uintptr_t>(sdp);

        if (sdp_addr >= mem::HHDM_BASE) {
            sdp_addr -= mem::HHDM_BASE;
        }

        *out_rsdp_address = static_cast<uacpi_phys_addr>(sdp_addr);
        return UACPI_STATUS_OK;
    }

#pragma region Memory

    void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len) {
        mem::VirtAddr virt_base = addr + mem::HHDM_BASE;

        uacpi_phys_addr phys_aligned = addr & ~0xFFFull;
        mem::VirtAddr virt_aligned = virt_base & ~0xFFFull;

        uacpi_size offset = addr - phys_aligned;
        uacpi_size num_pages = (len + offset + 0xFFF) / 0x1000;

        for (uacpi_size i = 0; i < num_pages; ++i) {
            mem::VirtAddr v = virt_aligned + (i * 0x1000);
            uacpi_phys_addr p = phys_aligned + (i * 0x1000);
            
            mem::PagingError err = mem::PagingBackend::map_page(v, p, mem::PageFlags::MMIO);
            
            if (err != mem::PagingError::Success && err != mem::PagingError::AlreadyMapped ) {
                kprintf(gui::LOG_ERROR, "Couldn't map 0x%x, paging error %u!\n", v, err);
                return nullptr;
            }
        }

        return reinterpret_cast<void*>(virt_base);
    }

    void uacpi_kernel_unmap(void *addr, uacpi_size len) {
        (void)addr;
        (void)len;
    }

    void *uacpi_kernel_alloc(uacpi_size size) {
        return kmalloc(size);
    }

    void uacpi_kernel_free(void *mem) {
        kfree(mem);
    }

#pragma endregion

    void uacpi_kernel_log(uacpi_log_level level, const uacpi_char* msg) {
        switch (level)
        {
            case uacpi_log_level::UACPI_LOG_DEBUG:
                kprintf("[uACPI DEBUG] %s", msg);
                break;
            case uacpi_log_level::UACPI_LOG_TRACE:
                kprintf("[uACPI TRACE] %s", msg);
                break;
            case uacpi_log_level::UACPI_LOG_INFO:
                kprintf(gui::LOG_INFO, "[uACPI INFO] %s", msg);
                break;
            case uacpi_log_level::UACPI_LOG_WARN:
                kprintf(gui::LOG_WARNING, "[uACPI WARNING] %s", msg);
                break;
            case uacpi_log_level::UACPI_LOG_ERROR:
                kprintf(gui::LOG_ERROR, "[uACPI ERROR] %s", msg);
                break;
            
            default:
                kprintf("%s", msg);
                break;
        }
    }

#pragma region PCI

    uacpi_status uacpi_kernel_pci_device_open(
        uacpi_pci_address address, uacpi_handle *out_handle
    ) {
        (void)address; (void)out_handle;
        return UACPI_STATUS_OK;
    }

    void uacpi_kernel_pci_device_close(uacpi_handle handle) {
        (void)handle;
    }

    uacpi_status uacpi_kernel_pci_read8(uacpi_handle device, uacpi_size offset, uacpi_u8 *value) {
        (void)device; (void)offset; (void)value;
        return UACPI_STATUS_OK;
    }

    uacpi_status uacpi_kernel_pci_read16(uacpi_handle device, uacpi_size offset, uacpi_u16 *value) {
        (void)device; (void)offset; (void)value;
        return UACPI_STATUS_OK;
    }

    uacpi_status uacpi_kernel_pci_read32(uacpi_handle device, uacpi_size offset, uacpi_u32 *value) {
        (void)device; (void)offset; (void)value;
        return UACPI_STATUS_OK;
    }

    uacpi_status uacpi_kernel_pci_write8(uacpi_handle device, uacpi_size offset, uacpi_u8 value) {
        (void)device; (void)offset; (void)value;
        return UACPI_STATUS_OK;
    }

    uacpi_status uacpi_kernel_pci_write16(uacpi_handle device, uacpi_size offset, uacpi_u16 value) {
        (void)device; (void)offset; (void)value;
        return UACPI_STATUS_OK;
    }

    uacpi_status uacpi_kernel_pci_write32(uacpi_handle device, uacpi_size offset, uacpi_u32 value) {
        (void)device; (void)offset; (void)value;
        return UACPI_STATUS_OK;
    }

#pragma endregion

#pragma region IO
    uacpi_status uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size len, uacpi_handle *out_handle) {
        (void)len;
        *out_handle = reinterpret_cast<uacpi_handle>(static_cast<uintptr_t>(base));
        return UACPI_STATUS_OK;
    }

    void uacpi_kernel_io_unmap(uacpi_handle handle) {
        (void)handle;
    }

    uacpi_status uacpi_kernel_io_read8(uacpi_handle handle, uacpi_size offset, uacpi_u8 *out_value) {
        uacpi_io_addr port = static_cast<uacpi_io_addr>(reinterpret_cast<uintptr_t>(handle)) + offset;
        *out_value = cpu::inb(port);
        return UACPI_STATUS_OK;
    }

    uacpi_status uacpi_kernel_io_read16(uacpi_handle handle, uacpi_size offset, uacpi_u16 *out_value) {
        uacpi_io_addr port = static_cast<uacpi_io_addr>(reinterpret_cast<uintptr_t>(handle)) + offset;
        *out_value = cpu::inw(port);
        return UACPI_STATUS_OK;
    }

    uacpi_status uacpi_kernel_io_read32(uacpi_handle handle, uacpi_size offset, uacpi_u32 *out_value) {
        uacpi_io_addr port = static_cast<uacpi_io_addr>(reinterpret_cast<uintptr_t>(handle)) + offset;
        *out_value = cpu::inl(port);
        return UACPI_STATUS_OK;
    }

    uacpi_status uacpi_kernel_io_write8(uacpi_handle handle, uacpi_size offset, uacpi_u8 in_value) {
        uacpi_io_addr port = static_cast<uacpi_io_addr>(reinterpret_cast<uintptr_t>(handle)) + offset;
        cpu::outb(port, in_value);
        return UACPI_STATUS_OK;
    }

    uacpi_status uacpi_kernel_io_write16(uacpi_handle handle, uacpi_size offset, uacpi_u16 in_value) {
        uacpi_io_addr port = static_cast<uacpi_io_addr>(reinterpret_cast<uintptr_t>(handle)) + offset;
        cpu::outw(port, in_value);
        return UACPI_STATUS_OK;
    }

    uacpi_status uacpi_kernel_io_write32(uacpi_handle handle, uacpi_size offset, uacpi_u32 in_value) {
        uacpi_io_addr port = static_cast<uacpi_io_addr>(reinterpret_cast<uintptr_t>(handle)) + offset;
        cpu::outl(port, in_value);
        return UACPI_STATUS_OK;
    }

#pragma endregion

#pragma region Time

    uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void) {
        return get_monotonic_ns();
    }

    void uacpi_kernel_stall(uacpi_u8 usec) {
        uacpi_u64 start = uacpi_kernel_get_nanoseconds_since_boot();
        uacpi_u64 wait_ns = usec * 1000ULL;
        
        while ((uacpi_kernel_get_nanoseconds_since_boot() - start) < wait_ns) {
            asm volatile("pause");
        }
    }

    void uacpi_kernel_sleep(uacpi_u64 msec) {
        uacpi_u64 start = uacpi_kernel_get_nanoseconds_since_boot();
        uacpi_u64 wait_ns = msec * 1000000ULL;
        
        // Since you don't have a thread scheduler yet, sleep is just a longer stall
        while ((uacpi_kernel_get_nanoseconds_since_boot() - start) < wait_ns) {
            asm volatile("pause");
        }
    }

#pragma endregion

    uacpi_handle uacpi_kernel_create_mutex(void) {
        return reinterpret_cast<uacpi_handle>(1);
    }

    void uacpi_kernel_free_mutex(uacpi_handle handle) {
        (void)handle;
    }

    uacpi_handle uacpi_kernel_create_event(void) {
        return reinterpret_cast<uacpi_handle>(1);
    }

    void uacpi_kernel_free_event(uacpi_handle handle) {
        (void)handle;
    }

    uacpi_thread_id uacpi_kernel_get_thread_id(void) {
        return reinterpret_cast<uacpi_thread_id>(1);
    }

    uacpi_interrupt_state uacpi_kernel_disable_interrupts(void) {
        cpu::CPU::disable_interrupts();
        return 0;
    }

    void uacpi_kernel_restore_interrupts(uacpi_interrupt_state state) {
        (void)state;
        cpu::CPU::enable_interrupts();
    }

    uacpi_status uacpi_kernel_acquire_mutex(uacpi_handle handle, uacpi_u16 timeout) {
        (void)handle; (void)timeout;
        return UACPI_STATUS_OK;
    }

    void uacpi_kernel_release_mutex(uacpi_handle handle) {
        (void)handle;
    }

    uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle handle, uacpi_u16 timeout) {
        (void)handle; (void)timeout;
        return UACPI_TRUE;
    }

    void uacpi_kernel_signal_event(uacpi_handle handle) {
        (void)handle;
    }

    void uacpi_kernel_reset_event(uacpi_handle handle) {
        (void)handle;
    }

    uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request *req) {
        (void)req;
        return UACPI_STATUS_OK;
    }

    uacpi_status uacpi_kernel_install_interrupt_handler(
        uacpi_u32 irq, uacpi_interrupt_handler handler, uacpi_handle ctx,
        uacpi_handle *out_irq_handle
    ) {
        uint8_t vector = static_cast<uint8_t>(CPU_IRQ_NUM + irq);

        UacpiHandlerEntry* entry = allocate_uacpi_entry(vector);
        if (!entry) return UACPI_STATUS_NO_RESOURCE_END_TAG;

        entry->handler = handler;
        entry->ctx = ctx;

        arch::IDT::register_interrupt_handler(vector, uacpi_interrupt_trampoline);

        if (!arch::IOAPIC::find_gsi_and_write_rte(vector)) {
            // Roll back so this slot is reusable and we don't leave a
            // dangling IDT entry with no backing RTE.
            arch::IDT::unregister_interrupt_handler(vector);
            entry->in_use = false;
            entry->handler = nullptr;
            entry->ctx = nullptr;
            entry->vector = 0;
            return UACPI_STATUS_NOT_FOUND;
        }

        *out_irq_handle = reinterpret_cast<uacpi_handle>(static_cast<uintptr_t>(vector));
        return UACPI_STATUS_OK;
    }

    uacpi_status uacpi_kernel_uninstall_interrupt_handler(
        uacpi_interrupt_handler handler, uacpi_handle irq_handle
    ) {
        uint8_t vector = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(irq_handle));

        UacpiHandlerEntry* entry = get_uacpi_entry(vector);
        if (!entry || entry->handler != handler)
            return UACPI_STATUS_INVALID_ARGUMENT;

        arch::IDT::unregister_interrupt_handler(vector);
        entry->in_use = false;
        entry->handler = nullptr;
        entry->ctx = nullptr;
        entry->vector = 0;
        return UACPI_STATUS_OK;
    }

    uacpi_handle uacpi_kernel_create_spinlock(void) {
        return reinterpret_cast<uacpi_handle>(1);
    }

    void uacpi_kernel_free_spinlock(uacpi_handle handle) {
        (void)handle;
    }

    uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle handle) {
        (void)handle;
        return 0;
    }

    void uacpi_kernel_unlock_spinlock(uacpi_handle handle, uacpi_cpu_flags flags) {
        (void)handle; (void)flags;
    }

    uacpi_status uacpi_kernel_schedule_work(
        uacpi_work_type type, uacpi_work_handler handler, uacpi_handle ctx
    ) {
        (void)type;
        (void)handler;
        (void)ctx;
        return UACPI_STATUS_OK;
    }

    uacpi_status uacpi_kernel_wait_for_work_completion(void) {
        return UACPI_STATUS_OK;
    }
}