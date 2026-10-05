// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef CPU_HPP
#define CPU_HPP

#include <stdint.h>
#include <arch/gdt.hpp>
#include <arch/tss.hpp>
#include <arch/interrupts/idt.hpp>

namespace cpu
{
    /// @brief Cached CPUID features and metadata
    struct cpuid_cache_t {
        // Metadata
        char vendor_id[13];
        uint32_t max_std_leaf;
        uint32_t max_ext_leaf;
        
        uint32_t family;
        uint32_t model;
        uint32_t stepping;
        uint32_t local_apic_id;

        uint32_t xsave_area_size;

        // Memory & Paging
        bool has_nx;
        bool has_pdpe1gb;
        bool has_pcid;
        bool has_invpcid;

        // Security Mitigations
        bool has_smep;
        bool has_smap;
        bool has_umip;
        bool has_pku;
        bool has_pks;

        // Math & Context Switching
        bool has_fpu;
        bool has_sse;
        bool has_sse2;
        bool has_sse3;
        bool has_ssse3;
        bool has_sse4_1;
        bool has_sse4_2;
        bool has_avx;
        bool has_avx2;
        bool has_xsave;
        bool has_fsgsbase;

        // Interrupts & Timers
        bool has_apic;
        bool has_tsc;
        bool has_tsc_deadline;
        bool has_rdtscp;
        bool has_invariant_tsc;
    };

    struct run_queue_t {

    }; // Placeholder

    class CPU {
    public:
    // == CPU local data ==
        CPU* self;
        
        uint64_t kernel_stack;
        uint64_t user_stack;
        
        uint8_t cpu_id;
        uint8_t lapic_id;
        
        run_queue_t run_queue;
        
        arch::GDT gdt;
        arch::TSS tss;
        arch::IDT idt;
        
        void* ist_stacks[NUM_STACKS];
        
        void* slab_magazine;

        cpuid_cache_t cpuid_cache;
    
        // == Inline Instance Access ==
        
        /// @brief Gets the current CPU instance via the GS segment
        static inline CPU* get() {
            CPU* current_cpu;
            asm volatile("mov %%gs:0, %0" : "=r"(current_cpu));
            return current_cpu;
        }

        // == Initialization ==
        
        /// @brief Configures control registers & MSRs to enable security and performance features. Caches needed CPUID info
        static void init_cpu(bool is_bsp);
        
        // == CPU State Control ==
        
        /// @brief Stops the cpu forever
        [[noreturn]] static inline void haltloop() {
            while (true) {
                asm volatile("hlt");
            }
        }

        /// @brief Enables interrupts
        static inline void enable_interrupts() {
            asm volatile("sti" ::: "memory");
        }

        /// @brief Disables interrupts
        static inline void disable_interrupts() {
            asm volatile("cli" ::: "memory");
        }

        /// @brief CPU pause instruction for tight spinloops
        static inline void pause() {
            asm volatile("pause" ::: "memory");
        }

        // == MSR & CPUID Instructions ==
        
        static inline uint64_t read_msr(uint32_t msr) noexcept {
            uint32_t low, high;
            asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
            return ((uint64_t)high << 32) | low;
        }

        static inline void write_msr(uint32_t msr, uint64_t value) noexcept {
            uint32_t low = value & 0xFFFFFFFF;
            uint32_t high = value >> 32;
            asm volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high) : "memory");
        }

        static inline void cpuid(uint32_t leaf, uint32_t subleaf, 
                                uint32_t *eax, uint32_t *ebx, 
                                uint32_t *ecx, uint32_t *edx) {
            asm volatile("cpuid"
                : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                : "a"(leaf), "c"(subleaf)
                : "memory");
        }

        // == Control Registers & Paging ==
        
        static inline uint64_t read_cr0() {
            uint64_t val;
            asm volatile("mov %%cr0, %0" : "=r"(val));
            return val;
        }

        static inline void write_cr0(uint64_t val) {
            asm volatile("mov %0, %%cr0" :: "r"(val) : "memory");
        }

        static inline uint64_t read_cr2() {
            uint64_t val;
            asm volatile("mov %%cr2, %0" : "=r"(val));
            return val;
        }

        static inline uint64_t read_cr3() {
            uint64_t val;
            asm volatile("mov %%cr3, %0" : "=r"(val));
            return val;
        }

        static inline void write_cr3(uint64_t val) {
            asm volatile("mov %0, %%cr3" :: "r"(val) : "memory");
        }

        static inline uint64_t read_cr4() {
            uint64_t val;
            asm volatile("mov %%cr4, %0" : "=r"(val));
            return val;
        }

        static inline void write_cr4(uint64_t val) {
            asm volatile("mov %0, %%cr4" :: "r"(val) : "memory");
        }

        /// @brief Invalidates a single page from the TLB
        static inline void invlpg(void* vaddr) {
            asm volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
        }

        // == Extended State (XSAVE) ==
        
        static inline void xsetbv(uint32_t ext_ctrl_reg, uint64_t value) {
            uint32_t low = value & 0xFFFFFFFF;
            uint32_t high = value >> 32;
            asm volatile("xsetbv" : : "c"(ext_ctrl_reg), "a"(low), "d"(high) : "memory");
        }
    };

    extern CPU bsp_cpu;
} // namespace cpu


#endif // CPU_HPP
