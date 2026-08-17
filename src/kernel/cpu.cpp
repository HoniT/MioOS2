// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// x86_64 CPU helper methods
// ========================================

#include <cpu.hpp>
#include <graphics/kernel_gui.hpp>
#include <mm/slub.hpp>

using namespace cpu;

cpu_info_t CPU::bsp_cpu = {0};
cpu_local_data_t* CPU::bsp_local_data = nullptr;


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

// Helper function to write to XCR0
static inline void xsetbv(uint32_t ext_ctrl_reg, uint64_t value) {
    uint32_t eax = static_cast<uint32_t>(value);
    uint32_t edx = static_cast<uint32_t>(value >> 32);
    asm volatile("xsetbv" :: "c"(ext_ctrl_reg), "a"(eax), "d"(edx));
}

void CPU::init_cpu_features_cache() {
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

    kprintf(gui::PrintTypes::LOG_INFO, "Cached needed CPU info:\n");
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

void CPU::init_advanced_features() {
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

    if (bsp_cpu.has_sse) {
        cr4 |= (1ULL << 9);  // OSFXSR
        cr4 |= (1ULL << 10); // OSXMMEXCPT
    }
    
    if (bsp_cpu.has_fsgsbase) {
        cr4 |= (1ULL << 16); // FSGSBASE
    }
    
    if (bsp_cpu.has_pcid) {
        cr4 |= (1ULL << 17); // PCIDE
    }
    
    if (bsp_cpu.has_xsave) {
        cr4 |= (1ULL << 18); // OSXSAVE
    }
    
    if (bsp_cpu.has_smep) {
        cr4 |= (1ULL << 20); // SMEP
    }
    
    if (bsp_cpu.has_smap) {
        cr4 |= (1ULL << 21); // SMAP
    }

    cr4 |= (1ULL << 7); // PGE

    if (bsp_cpu.has_umip) {
        cr4 |= (1ULL << 11); // UMIP
    }

    asm volatile("mov %0, %%cr4" :: "r"(cr4) : "memory");


    
    // Configure the Page Attribute Table (PAT)
    // PAT0: WB, PAT1: WT, PAT2: UC-, PAT3: UC
    // PAT4: WB, PAT5: WT, PAT6: WC,  PAT7: UC
    uint64_t pat = 0x0001070600070406ULL;
    write_msr(0x277, pat);


    if (bsp_cpu.has_xsave) {
        uint64_t xcr0 = 0;
        xcr0 |= (1ULL << 0); // X87
        xcr0 |= (1ULL << 1); // SSE
        
        if (bsp_cpu.has_avx) {
            xcr0 |= (1ULL << 2); // AVX
        }
        
        xsetbv(0, xcr0);
    }

    kprintf(gui::LOG_INFO, "Set up advanced CPU state\n");
}

extern "C" uint8_t stack_top[];

void CPU::late_init_advanced_features() {
    bsp_local_data = (cpu_local_data_t*)kmalloc(sizeof(cpu_local_data_t));
    if(bsp_local_data == nullptr) {
        kprintf(gui::LOG_ERROR, "Couldn't allocate memory for BSP local data\n");
        return;
    }

    bsp_local_data->kernel_stack = reinterpret_cast<uint64_t>(stack_top) + mem::HHDM_BASE;
    bsp_local_data->user_stack = 0;
    bsp_local_data->current_thread = nullptr;
    bsp_local_data->cpu_id = 0;
    bsp_local_data->lapic_id = bsp_cpu.local_apic_id;

    // GS.base
    write_msr(0xC0000101, reinterpret_cast<uint64_t>(bsp_local_data));
    // KernelGSbase
    write_msr(0xC0000102, 0);

    kprintf(gui::LOG_INFO, "Fully initialized the CPU state (BSP local data: 0x%x)\n", bsp_local_data);
}
