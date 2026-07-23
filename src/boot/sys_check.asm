; ========================================
; Copyright Ioane Baidoshvili 2026.
; Distributed under the terms of the MIT License.
;
; System hardware checks
; ========================================

section .boot.rodata

err_msg_cpuid:     db "CPUID is not supported on this CPU", 0
err_msg_ext_cpuid: db "Extended CPUID is not supported on this CPU", 0
err_msg_long_mode: db "Long mode is not supported on this CPU", 0
err_msg_features:  db "Missing baseline 64-bit features on this CPU", 0

section .boot.text
[bits 32]

global check_cpuid
global check_extended_cpuid
global check_long_mode
global check_cpu_features

extern log_error

check_cpuid:
    pushfd
    pop eax

    mov ecx, eax
    ; Flip the ID bit (bit 21)
    xor eax, 1 << 21

    push eax
    popfd

    ; It should push a reverted eflags at this point if cpuid isn't supported
    pushfd
    pop eax

    ; Restore the original EFLAGS from ECX
    push ecx
    popfd

    ; Compare EAX (what EFLAGS is now) with ECX (what EFLAGS was originally)
    cmp eax, ecx
    je .no_cpuid
    ret

.no_cpuid:
    mov esi, err_msg_cpuid
    jmp log_error

check_extended_cpuid:
    ; Set EAX to 0x80000000 to ask for the highest extended function
    mov eax, 0x80000000
    cpuid

    ; Check if the maximum extended function is at least 0x80000001
    cmp eax, 0x80000001
    jb .no_extended_cpuid
    ret

.no_extended_cpuid:
    mov esi, err_msg_ext_cpuid
    jmp log_error

check_long_mode:
    ; Ask for extended processor info
    mov eax, 0x80000001
    cpuid

    ; Test if bit 29 in EDX is set (the LM bit)
    test edx, 1 << 29
    jz .no_long_mode
    ret

.no_long_mode:
    mov esi, err_msg_long_mode
    jmp log_error

check_cpu_features:
    mov eax, 1
    cpuid

    ; FPU  (bit 0)
    ; MSR  (bit 5)
    ; PAE  (bit 6)
    ; FXSR (bit 24)
    ; SSE  (bit 25)
    ; SSE2 (bit 26)
    ;
    ; Combined Bitmask: 0x07000061
    mov eax, 0x07000061
    mov ecx, edx
    and ecx, eax      ; Isolate only the bits we care about
    cmp ecx, eax      ; Check if all required bits are set to 1
    jne .no_features
    ret

.no_features:
    mov esi, err_msg_features
    jmp log_error
