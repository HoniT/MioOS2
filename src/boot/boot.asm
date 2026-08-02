; ========================================
; Copyright Ioane Baidoshvili 2026.
; Distributed under the terms of the MIT License.
;
; Main bootloader file
; ========================================

; GRUB Multiboot2 headers

section .multiboot_header
align 8
header_start:
    dd 0xE85250D6 ; Magic
    dd 0
    dd header_end - header_start
    dd -(0xE85250D6 + 0 + (header_end - header_start)) ; Checksum

    align 8
    ; Framebuffer tag
    dw 5
    dw 1
    dd 20
    dd 0
    dd 0
    dd 0

    align 8
    ; End tag
    dw 0
    dw 0
    dd 0
header_end:

section .boot.bss nobits

global p4_table
global p3_table
align 4096
p4_table: resb 4096
p3_table: resb 4096
p2_table: resb 4096

; Kernel stack
align 16
global stack_top
stack_bottom: resb 16384 ; 16KiB
stack_top:

section .boot.rodata
err_msg_multiboot: db "OS wasn't booted from Multiboot2-compliant bootloader", 0

section .boot.text
[bits 32]

extern log_error
extern check_cpuid
extern check_extended_cpuid
extern check_long_mode
extern check_cpu_features

; Early boot

global _start
_start:
    cli

    mov esp, stack_top

    mov edi, ebx ; Multiboot info pointer (Physical)
    mov esi, eax ; Magic number
    ; Verifying GRUB magic
    cmp esi, 0x36D76289
    jne .no_magic

    call check_cpuid
    call check_extended_cpuid
    call check_long_mode
    call check_cpu_features

    mov eax, p3_table
    or eax, 0b11 ; Present, Writable
    
    ; 1. Identity Map (PML4[0] -> PDP)
    mov [p4_table], eax

    ; 2. HHDM Map (PML4[256] -> PDP) - Maps 0xFFFF800000000000
    mov [p4_table + 256 * 8], eax

    ; 3. Kernel Map (PML4[511] -> PDP) - Maps 0xFFFFFFFF80000000
    mov [p4_table + 511 * 8], eax

    mov eax, p2_table
    or eax, 0b11 ; Present, Writable
    mov [p3_table + 510 * 8], eax
    
    ; Also map PDP[0] to PD for the Identity map AND HHDM map
    mov [p3_table], eax

    ; Map the PD table using 2MB huge pages (First 1 GiB)
    mov ecx, 0

.map_p2_table:
    mov eax, 0x200000
    mul ecx
    or eax, 0b10000011 ; Present, Writable, Huge Page
    mov [p2_table + ecx * 8], eax
    inc ecx
    cmp ecx, 512
    jne .map_p2_table

    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Load PML4
    mov eax, p4_table
    mov cr3, eax

    ; Enable Long Mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable Paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; ENABLE SSE
    mov eax, cr0
    and ax, 0xFFFB
    or ax, 0x2
    mov cr0, eax
    mov eax, cr4
    or ax, 3 << 9
    mov cr4, eax

    ; Long mode
    lgdt [gdt64.pointer]
    jmp gdt64.code_segment:realm64

.no_magic:
    mov esi, err_msg_multiboot
    jmp log_error

; 64 bit realm

[bits 64]
extern kernel_main
extern KERNEL_VMA
realm64:
    ; Clear old segment registers
    mov ax, 0
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; HIGHER HALF SHIFT & KERNEL HANDOFF
    
    ; Shift the stack pointer into the Kernel Base
    mov rax, KERNEL_VMA
    add rsp, rax 

    ; Shift the Multiboot pointer (rdi) into the HHDM 
    mov rax, 0xFFFF800000000000
    add rdi, rax 

    mov rax, kernel_main
    call rax

    cli
.hang
    cli
    hlt
    jmp .hang

section .boot.data
align 8

; A temporary GDT
gdt64:
    dq 0 ; Null descriptor
.code_segment: equ $ - gdt64
    ; Executable | Descriptor | Present | 64-bit flag
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) 
.pointer:
    dw $ - gdt64 - 1
    dq gdt64