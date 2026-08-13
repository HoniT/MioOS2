// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef CPU_HPP
#define CPU_HPP

#include <stdint.h>

namespace cpu
{
    typedef struct {
        // Metadata
        char vendor_id[13];
        uint32_t max_std_leaf;
        uint32_t max_ext_leaf;
        
        uint32_t family;
        uint32_t model;
        uint32_t stepping;
        uint32_t local_apic_id;

        // Memory & Paging
        bool has_nx;
        bool has_pdpe1gb;
        bool has_pcid;
        bool has_invpcid;

        // Security Mitigations
        bool has_smep;
        bool has_smap;
        bool has_umip;

        // Math & Context Switching
        bool has_fpu;
        bool has_sse;
        bool has_sse2;
        bool has_avx;
        bool has_avx2;
        bool has_xsave;
        bool has_fsgsbase;
        bool has_apic;
    } cpu_info_t;

    struct thread_t {

    }; // Placeholder

    struct cpu_local_data_t {
        uint64_t kernel_stack;  // Offset 0x00: Loaded by syscall handler
        uint64_t user_stack;    // Offset 0x08: Saved by syscall handler
        thread_t* current_thread;
        uint32_t cpu_id;
        uint32_t lapic_id;
    };

    class CPU {
    private:
        static cpu_info_t bsp_cpu;
        static cpu_local_data_t* bsp_local_data;
    public:
        /// @brief Stops the cpu forever
        [[noreturn]] static void haltloop();
    
        /// @brief Enables interrupts
        static void enable_interrupts();
    
        /// @brief Disables interrupts
        static void disable_interrupts();

        static uint64_t read_msr(uint32_t msr) noexcept;
        static void write_msr(uint32_t msr, uint64_t value) noexcept;

        static cpu_info_t get_bsp_cpu() { return bsp_cpu; }
        static void init_cpu_features_cache();

        /// @brief Configures control registers & MSRs to enable security and performance features
        static void init_advanced_features();
        /// @brief Fully initializes the CPU after memory management is initialized
        static void late_init_advanced_features();

        static void cpuid(uint32_t leaf, uint32_t subleaf, 
                         uint32_t *eax, uint32_t *ebx, 
                         uint32_t *ecx, uint32_t *edx);
    };
} // namespace cpu


#endif // CPU_HPP
