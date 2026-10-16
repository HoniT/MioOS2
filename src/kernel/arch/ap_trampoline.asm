; ========================================
; Copyright Ioane Baidoshvili 2026.
; Distributed under the terms of the MIT License.
;
; AP trampoline code
; ========================================

global trampoline_start
global trampoline_end
global trampoline_gdt
global trampoline32
global trampoline64

section .rodata
align 16

[bits 16]
trampoline_start:
    cli
    cld

    ; Sync Data Segment to Code Segment (set by SIPI vector)
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax

    ; Load the physical base into EBX while DS is valid for local offsets
    mov ebx, dword [trampoline_data_phys_base - trampoline_start]

    ; Load 32-bit GDT stub
    lgdt [trampoline_gdt_ptr - trampoline_start]

    ; Enable Protected Mode
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ; Far jump into 32-bit mode using memory pointer
    jmp dword far [trampoline_data_32_ptr - trampoline_start]

[bits 32]
trampoline32:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; Enable PAE in CR4
    mov eax, cr4
    or eax, (1 << 5)
    mov cr4, eax

    ; EBX contains the physical base from 16-bit mode. Access data relative to it.
    mov eax, dword [ebx + (trampoline_data_cr3 - trampoline_start)]
    mov cr3, eax

    ; Enable Long Mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 8) | (1 << 11) | (1 << 0)   ; LME | NXE | SCE
    wrmsr

    ; Enable Paging
    mov eax, cr0
    or eax, (1 << 31)
    mov cr0, eax

    ; Far jump into 64-bit mode using base-relative address
    jmp dword far [ebx + (trampoline_data_64_ptr - trampoline_start)]

[bits 64]
trampoline64:
    ; EBX is zero-extended to RBX. Use it to offset the absolute physical addresses.
    
    ; Load the pre-allocated stack assigned by the BSP
    mov rsp, qword [rbx + (trampoline_data_rsp - trampoline_start)]
    
    ; Load the CPU object pointer into RDI
    mov rdi, qword [rbx + (trampoline_data_cpu_ptr - trampoline_start)]

    ; Jump to ap_entry(cpu::CPU* cpu_obj)
    mov rax, qword [rbx + (trampoline_data_entry - trampoline_start)]
    call rax

.halt:
    cli
    hlt
    jmp .halt

align 16
trampoline_gdt:
    dq 0x0000000000000000 ; Null
    dq 0x00CF9A000000FFFF ; 32-bit Code (0x08)
    dq 0x00CF92000000FFFF ; 32-bit Data (0x10)
    dq 0x00AF9A000000FFFF ; 64-bit Code (0x18)
    dq 0x00AF92000000FFFF ; 64-bit Data (0x20)

trampoline_gdt_ptr:
    dw (trampoline_gdt_ptr - trampoline_gdt - 1)
    dq 0

; --- Patching area written to by BSP ---
align 8
global trampoline_data_cr3
global trampoline_data_rsp
global trampoline_data_entry
global trampoline_data_cpu_ptr
global trampoline_data_32_addr
global trampoline_data_64_addr
global trampoline_data_phys_base
global trampoline_gdt_ptr

trampoline_data_cr3:       dq 0
trampoline_data_rsp:       dq 0
trampoline_data_entry:     dq 0
trampoline_data_cpu_ptr:   dq 0

; 6-byte far pointers
trampoline_data_32_ptr:
trampoline_data_32_addr:   dd 0
trampoline_data_32_seg:    dw 0x08

trampoline_data_64_ptr:
trampoline_data_64_addr:   dd 0
trampoline_data_64_seg:    dw 0x18

trampoline_data_phys_base: dd 0

trampoline_end: