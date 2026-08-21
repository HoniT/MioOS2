; ========================================
; Copyright Ioane Baidoshvili 2026.
; Distributed under the terms of the MIT License.
;
; Syscall entry asm
; ========================================

[bits 64]

section .text
global syscall_entry
extern syscall_table
extern __NR_syscall_max

%define ENOSYS 38

syscall_entry:
    ; Entering ring 0

    ; Swap GS to point to kernel's cpu_local_data_t instead of user's GS
    swapgs

    mov [gs:0x08], rsp
    mov rsp, [gs:0x00]

    ; Save the CPU state
    ; Build the register state on the kernel stack
    push qword [gs:0x08]            ; Saved user RSP
    push r11                        ; User RFLAGS
    push rcx                        ; User RIP

    push rax                        ; Syscall number
    push rdi                        ; 1st argument
    push rsi                        ; 2nd argument
    push rdx                        ; 3rd argument
    push r10                        ; 4th argument
    push r8                         ; 5th argument
    push r9                         ; 6th argument
    push rbx                        ; Callee-saved register
    push rbp                        ; Callee-saved register
    push r12                        ; Callee-saved register
    push r13                        ; Callee-saved register
    push r14                        ; Callee-saved register
    push r15                        ; Callee-saved register

    mov r11, rax
    
    ; Ensure syscall number is within the valid range
    cmp r11, qword [__NR_syscall_max]
    ja  .bad

    ; System V ABI requires the 4th arg in RCX, but syscall puts it in R10
    mov rcx, r10                    
    sti
    
    call [syscall_table + r11*8] ; Call the CPP function
    
    ; Disable interrupts again before we start manipulating the stack 
    ; and GS base to return to user mode.
    cli                             
    jmp .done

.bad:
    mov rax, -ENOSYS ; I'm just gonna use the same as Linux's ENOSYS because why not -_-

.done:
    ; Restore state and return
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    pop r9
    pop r8
    pop r10
    pop rdx
    pop rsi
    pop rdi
    
    ; Skip the saved RAX to not overwrite it
    add rsp, 8

    ; Restore the registers required for the sysret instruction
    pop rcx
    pop r11
    pop rsp

    ; Return to user mode
    swapgs
    o64 sysret
