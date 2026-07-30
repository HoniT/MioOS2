; ========================================
; Copyright Ioane Baidoshvili 2026.
; Distributed under the terms of the MIT License.
;
; Flushes GDT and reloads segment registers
; ========================================

[bits 64]

section .text

global gdt_flush
gdt_flush:
    ; Flushing the GDT
    lgdt [rdi]

    ; Reloading segment registers
    push 0x08
    lea rax, [rel .reload_cs]
    push rax
    retfq

    .reload_cs:
        mov ax, 0x10
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax
        ret

global tss_flush
tss_flush:
    ltr di
    ret
