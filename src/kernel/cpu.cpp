// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// x86_64 CPU helper methods
// ========================================

#include <cpu.hpp>
#include <graphics/kprint.hpp>

using namespace cpu;

cpu_info_t CPU::bsp_cpu = {0};

[[noreturn]] void CPU::haltloop() {
    for(;;) {
        asm volatile("cli");
        asm volatile("hlt");
    }
}

void CPU::cpuid(uint32_t leaf, uint32_t subleaf, 
                         uint32_t *eax, uint32_t *ebx, 
                         uint32_t *ecx, uint32_t *edx) {
    asm volatile(
        "cpuid"
        : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
        : "a" (leaf), "c" (subleaf)
        : "memory"
    );
}

void CPU::enable_interrupts() { asm volatile("sti"); }

void CPU::disable_interrupts() { asm volatile("cli"); }

// Reads a 64-bit value from a Model-Specific Register
uint64_t CPU::read_msr(uint32_t msr) noexcept {
    uint32_t low, high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return (static_cast<uint64_t>(high) << 32) | low;
}

// Writes a 64-bit value to a Model-Specific Register
void CPU::write_msr(uint32_t msr, uint64_t value) noexcept {
    uint32_t low = static_cast<uint32_t>(value);
    uint32_t high = static_cast<uint32_t>(value >> 32);
    asm volatile("wrmsr" : : "a"(low), "d"(high), "c"(msr));
}

void CPU::init_cpu_cache() {
    uint32_t eax, ebx, ecx, edx;

    // Max Standard Leaf & Vendor ID
    cpuid(0, 0, &eax, &ebx, &ecx, &edx);
    bsp_cpu.max_std_leaf = eax;
    
    uint32_t *vendor = (uint32_t *)bsp_cpu.vendor_id;
    vendor[0] = ebx;
    vendor[1] = edx;
    vendor[2] = ecx;
    bsp_cpu.vendor_id[12] = '\0';

    // Max Extended Leaf
    cpuid(0x80000000, 0, &eax, &ebx, &ecx, &edx);
    bsp_cpu.max_ext_leaf = eax;

    // Basic Features & Signature
    if (bsp_cpu.max_std_leaf >= 1) {
        cpuid(1, 0, &eax, &ebx, &ecx, &edx);
        
        bsp_cpu.stepping = eax & 0x0F;
        bsp_cpu.model    = (eax >> 4) & 0x0F;
        bsp_cpu.family   = (eax >> 8) & 0x0F;
        bsp_cpu.local_apic_id = (ebx >> 24) & 0xFF;

        bsp_cpu.has_fpu   = (edx & (1 << 0))  != 0;
        bsp_cpu.has_apic  = (edx & (1 << 9))  != 0;
        bsp_cpu.has_sse   = (edx & (1 << 25)) != 0;
        bsp_cpu.has_sse2  = (edx & (1 << 26)) != 0;
        
        bsp_cpu.has_pcid  = (ecx & (1 << 17)) != 0;
        bsp_cpu.has_xsave = (ecx & (1 << 26)) != 0;
        bsp_cpu.has_avx   = (ecx & (1 << 28)) != 0;
    }

    // Extended Features
    if (bsp_cpu.max_std_leaf >= 7) {
        cpuid(7, 0, &eax, &ebx, &ecx, &edx);
        
        bsp_cpu.has_fsgsbase = (ebx & (1 << 0))  != 0;
        bsp_cpu.has_smep     = (ebx & (1 << 7))  != 0;
        bsp_cpu.has_smap     = (ebx & (1 << 20)) != 0;
        bsp_cpu.has_avx2     = (ebx & (1 << 5))  != 0;
        bsp_cpu.has_invpcid  = (ebx & (1 << 10)) != 0;
        
        bsp_cpu.has_umip     = (ecx & (1 << 2))  != 0;
    }

    // Extended Processor Info
    if (bsp_cpu.max_ext_leaf >= 0x80000001) {
        cpuid(0x80000001, 0, &eax, &ebx, &ecx, &edx);
        
        bsp_cpu.has_nx      = (edx & (1 << 20)) != 0;
        bsp_cpu.has_pdpe1gb = (edx & (1 << 26)) != 0;
    }

    klogf("CPU", "Cached needed CPU info:\n");
    kprintf("   Vendor ID:     %s\n", bsp_cpu.vendor_id);
    kprintf("   Max Std Leaf:  0x%u\n", bsp_cpu.max_std_leaf);
    kprintf("   Max Ext Leaf:  0x%u\n", bsp_cpu.max_ext_leaf);
    kprintf("   Family:        %u\n", bsp_cpu.family);
    kprintf("   Model:         %u\n", bsp_cpu.model);
    kprintf("   Stepping:      %u\n", bsp_cpu.stepping);
    kprintf("   Local APIC ID: %u\n", bsp_cpu.local_apic_id);

    kprintf("   NX/NXE Bit:    %u\n", bsp_cpu.has_nx);
    kprintf("   1GB Pages:     %u\n", bsp_cpu.has_pdpe1gb);
    kprintf("   PCID:          %u\n", bsp_cpu.has_pcid);
    kprintf("   INVPCID:       %u\n", bsp_cpu.has_invpcid);

    kprintf("   SMEP:          %u\n", bsp_cpu.has_smep);
    kprintf("   SMAP:          %u\n", bsp_cpu.has_smap);
    kprintf("   UMIP:          %u\n", bsp_cpu.has_umip);

    kprintf("   FPU:           %u\n", bsp_cpu.has_fpu);
    kprintf("   APIC:          %u\n", bsp_cpu.has_apic);
    kprintf("   SSE:           %u\n", bsp_cpu.has_sse);
    kprintf("   SSE2:          %u\n", bsp_cpu.has_sse2);
    kprintf("   AVX:           %u\n", bsp_cpu.has_avx);
    kprintf("   AVX2:          %u\n", bsp_cpu.has_avx2);
    kprintf("   XSAVE:         %u\n", bsp_cpu.has_xsave);
    kprintf("   FSGSBASE:      %u\n", bsp_cpu.has_fsgsbase);
}
