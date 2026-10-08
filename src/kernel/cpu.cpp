// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// x86_64 CPU & CPUID helper methods
// ========================================

#include <cpu.hpp>
#include <kernel_ui.hpp>
#include <mm/slub.hpp>
#include <mm/pmm.hpp>

using namespace cpu;

CPU cpu::bsp_cpu;

static volatile uint8_t next_cpu_id = 1;

void CPU::init_cpu(bool is_bsp) {
    if (is_bsp) {
        this->cpu_id = 0;
        this->kernel_stack = mem::get_kstack_top();
    } else {
        this->cpu_id = next_cpu_id++;
        this->kernel_stack = (uint64_t)kmalloc(16384) + 16384;
    }

    this->self = this;
    this->user_stack = 0;

    write_msr(0xC0000101, reinterpret_cast<uint64_t>(this));
    write_msr(0xC0000102, 0); // KernelGSbase

    cpuid_cache_t& cache = this->cpuid_cache;

    uint32_t eax, ebx, ecx, edx;

    // Max Standard Leaf & Vendor ID
    cpuid(0, 0, &eax, &ebx, &ecx, &edx);
    cache.max_std_leaf = eax;
    
    uint32_t *vendor = (uint32_t *)cache.vendor_id;
    vendor[0] = ebx;
    vendor[1] = edx;
    vendor[2] = ecx;
    cache.vendor_id[12] = '\0';

    // Max Extended Leaf
    cpuid(0x80000000, 0, &eax, &ebx, &ecx, &edx);
    cache.max_ext_leaf = eax;

    // Basic Features & Signature
    if (cache.max_std_leaf >= 1) {
        cpuid(1, 0, &eax, &ebx, &ecx, &edx);
        
        cache.stepping      = eax & 0x0F;
        cache.model         = (eax >> 4) & 0x0F;
        cache.family        = (eax >> 8) & 0x0F;
        cache.local_apic_id = (ebx >> 24) & 0xFF;

        // EDX Features
        cache.has_fpu       = (edx & (1 << 0))  != 0;
        cache.has_tsc       = (edx & (1 << 4))  != 0;
        cache.has_apic      = (edx & (1 << 9))  != 0;
        cache.has_sse       = (edx & (1 << 25)) != 0;
        cache.has_sse2      = (edx & (1 << 26)) != 0;
        
        // ECX Features
        cache.has_sse3      = (ecx & (1 << 0))  != 0;
        cache.has_ssse3     = (ecx & (1 << 9))  != 0;
        cache.has_pcid      = (ecx & (1 << 17)) != 0;
        cache.has_sse4_1    = (ecx & (1 << 19)) != 0;
        cache.has_sse4_2    = (ecx & (1 << 20)) != 0;
        cache.has_tsc_deadline = (ecx & (1 << 24)) != 0;
        cache.has_xsave     = (ecx & (1 << 26)) != 0;
        cache.has_avx       = (ecx & (1 << 28)) != 0;
    }

    // Extended Features
    if (cache.max_std_leaf >= 7) {
        cpuid(7, 0, &eax, &ebx, &ecx, &edx);
        
        // EBX Features
        cache.has_fsgsbase  = (ebx & (1 << 0))  != 0;
        cache.has_pku       = (ebx & (1 << 3))  != 0;
        cache.has_avx2      = (ebx & (1 << 5))  != 0;
        cache.has_smep      = (ebx & (1 << 7))  != 0;
        cache.has_invpcid   = (ebx & (1 << 10)) != 0;
        cache.has_smap      = (ebx & (1 << 20)) != 0;
        
        // ECX Features
        cache.has_umip      = (ecx & (1 << 2))  != 0;
        cache.has_pks       = (ecx & (1 << 30)) != 0;
    }

    // XSAVE Area Information
    if (cache.has_xsave && cache.max_std_leaf >= 0x0D) {
        cpuid(0x0D, 0, &eax, &ebx, &ecx, &edx);
        cache.xsave_area_size = ecx; 
    } else {
        cache.xsave_area_size = 512; // Fallback for standard FXSAVE (x87/SSE)
    }

    // Extended Processor Info
    if (cache.max_ext_leaf >= 0x80000001) {
        cpuid(0x80000001, 0, &eax, &ebx, &ecx, &edx);
        
        cache.has_nx        = (edx & (1 << 20)) != 0;
        cache.has_pdpe1gb   = (edx & (1 << 26)) != 0;
        cache.has_rdtscp    = (edx & (1 << 27)) != 0;
    }

    // Advanced Power Management (Invariant TSC)
    if (cache.max_ext_leaf >= 0x80000007) {
        cpuid(0x80000007, 0, &eax, &ebx, &ecx, &edx);
        cache.has_invariant_tsc = (edx & (1 << 8)) != 0;
    }

    this->lapic_id = cache.local_apic_id;

    // Logging
    kprintf(gui::PrintTypes::LOG_INFO, "Cached CPU info for Core %u (LAPIC: %u):\n", 
            this->cpu_id, cache.local_apic_id);
            
    kprintf("   Vendor ID:     %s\n", cache.vendor_id);
    kprintf("   Family: %u Model: %u Stepping: %u\n", cache.family, cache.model, cache.stepping);

    kprintf("   [Memory & Paging]\n");
    kprintf("     NX/NXE: %u | 1GB Pages: %u | PCID: %u | INVPCID: %u\n", 
            cache.has_nx, cache.has_pdpe1gb, cache.has_pcid, cache.has_invpcid);

    kprintf("   [Security Mitigations]\n");
    kprintf("     SMEP: %u | SMAP: %u | UMIP: %u | PKU: %u | PKS: %u\n", 
            cache.has_smep, cache.has_smap, cache.has_umip, cache.has_pku, cache.has_pks);

    kprintf("   [Math & SIMD]\n");
    kprintf("     FPU: %u | SSE: %u | SSE2: %u | SSE3: %u | SSSE3: %u\n", 
            cache.has_fpu, cache.has_sse, cache.has_sse2, cache.has_sse3, cache.has_ssse3);
    kprintf("     SSE4.1: %u | SSE4.2: %u | AVX: %u | AVX2: %u\n", 
            cache.has_sse4_1, cache.has_sse4_2, cache.has_avx, cache.has_avx2);
    kprintf("     XSAVE: %u (Size: %u bytes) | FSGSBASE: %u\n", 
            cache.has_xsave, cache.xsave_area_size, cache.has_fsgsbase);
            
    kprintf("   [Interrupts & Timers]\n");
    kprintf("     APIC: %u | TSC: %u | TSC-Deadline: %u | RDTSCP: %u | Invariant TSC: %u\n", 
            cache.has_apic, cache.has_tsc, cache.has_tsc_deadline, cache.has_rdtscp, cache.has_invariant_tsc);



    uint64_t cr0;
    uint64_t cr4;

    // Configuring CR0
    asm volatile("mov %%cr0, %0" : "=r"(cr0));

    // Set MP (bit 1), NE (bit 5), WP (bit 16)
    cr0 |= (1ULL << 1) | (1ULL << 5) | (1ULL << 16);
    // Clear EM (bit 2), TS (bit 3)
    cr0 &= ~((1ULL << 2) | (1ULL << 3));

    asm volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");



    // Configuring CR4
    asm volatile("mov %%cr4, %0" : "=r"(cr4));

    if (cache.has_sse) {
        cr4 |= (1ULL << 9);  // OSFXSR
        cr4 |= (1ULL << 10); // OSXMMEXCPT
    }
    
    if (cache.has_fsgsbase) {
        cr4 |= (1ULL << 16); // FSGSBASE
    }
    
    if (cache.has_pcid) {
        cr4 |= (1ULL << 17); // PCIDE
    }
    
    if (cache.has_xsave) {
        cr4 |= (1ULL << 18); // OSXSAVE
    }
    
    if (cache.has_smep) {
        cr4 |= (1ULL << 20); // SMEP
    }
    
    if (cache.has_smap) {
        cr4 |= (1ULL << 21); // SMAP
    }

    cr4 |= (1ULL << 7); // PGE

    if (cache.has_umip) {
        cr4 |= (1ULL << 11); // UMIP
    }

    asm volatile("mov %0, %%cr4" :: "r"(cr4) : "memory");


    
    // Configure the Page Attribute Table (PAT)
    // PAT0: WB, PAT1: WT, PAT2: UC-, PAT3: UC
    // PAT4: WB, PAT5: WT, PAT6: WC,  PAT7: UC
    uint64_t pat = 0x0001070600070406ULL;
    write_msr(0x277, pat);


    if (cache.has_xsave) {
        uint64_t xcr0 = 0;
        xcr0 |= (1ULL << 0); // X87
        xcr0 |= (1ULL << 1); // SSE
        
        if (cache.has_avx) {
            xcr0 |= (1ULL << 2); // AVX
        }
        
        xsetbv(0, xcr0);
    }

    kprintf(gui::LOG_INFO, "Set up CPU state\n");
}
