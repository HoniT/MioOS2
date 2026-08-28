; ========================================
; Copyright Ioane Baidoshvili 2026.
; Distributed under the terms of the MIT License.
;
; Defines ISR macros and common stubs
; ========================================

[bits 64]

section .text

global idt_flush
idt_flush:
    lidt [rdi]
    ; I have a problem with the PIT freezing when I'm initializing the buddy allocator. So I'm gonna call sti manually somewhere -_- osdev is hard af
    ; sti
    ret

%macro ISR_NOERRCODE 1
    global isr%1
    isr%1:
        cli
        push qword 0 ; No error code
        push qword %1
        jmp cpu_isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
    global isr%1
    isr%1:
        cli
        push qword %1
        jmp cpu_isr_common_stub
%endmacro

; CPU Exceptions (Vectors 0-31)

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE 8
ISR_NOERRCODE 9
ISR_ERRCODE 10
ISR_ERRCODE 11
ISR_ERRCODE 12
ISR_ERRCODE 13
ISR_ERRCODE 14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_ERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_ERRCODE   29
ISR_ERRCODE   30
ISR_NOERRCODE 31

; Hardware Interrupts (Vectors 32-255)

%assign i 32
%rep 224
    global isr %+ i
    isr %+ i:
        cli
        push qword 0 ; No error code
        push qword i
        jmp hw_isr_common_stub
%assign i i+1
%endrep

section .data

; Table of exception stubs (0-31)
global cpu_isr_stub_table
cpu_isr_stub_table:
%assign i 0
%rep 32
    dq isr %+ i
%assign i i+1
%endrep

; Table of hardware interrupt stubs (32-255)
global hw_isr_stub_table
hw_isr_stub_table:
%assign i 32
%rep 224
    dq isr %+ i
%assign i i+1
%endrep

section .text


extern cpu_irq_handler
cpu_isr_common_stub:
    ; InterruptRegisters in idt.hpp
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp

    call cpu_irq_handler

    ; Restore registers (in exact reverse order)
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax

    add rsp, 16
    iretq


extern hw_irq_handler
hw_isr_common_stub:
    ; InterruptRegisters in idt.hpp
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp

    call hw_irq_handler

    ; Restore registers (in exact reverse order)
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax

    add rsp, 16
    iretq
