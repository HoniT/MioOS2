; ========================================
; Copyright Ioane Baidoshvili 2026.
; Distributed under the terms of the MIT License.
;
; Serial Port (COM1) error logger
; ========================================

section .boot.rodata
err_prefix db "BOOT ERROR: ", 0

section .boot.text
[bits 32]

global log_error

PORT equ 0x3F8

; Input: ESI must point to a null-terminated string
log_error:
    ; Initialize the Serial Port (since we're using framebuffer now)
    mov dx, PORT + 1
    mov al, 0x00
    out dx, al

    mov dx, PORT + 3
    mov al, 0x80
    out dx, al

    mov dx, PORT + 0
    mov al, 0x03
    out dx, al

    mov dx, PORT + 1
    mov al, 0x00
    out dx, al

    mov dx, PORT + 3
    mov al, 0x03
    out dx, al

    mov dx, PORT + 2
    mov al, 0xC7
    out dx, al

    mov dx, PORT + 4
    mov al, 0x0B
    out dx, al

    ; Save the user's string pointer
    push esi
    
    ; Load and print the prefix
    mov esi, err_prefix
    call .print_string

    ; Restore the user's string pointer and print it
    pop esi
    call .print_string

.hang:
    cli
    hlt
    jmp .hang

.print_string:
.print_loop:
    mov al, [esi]
    
    ; Check if it is the null terminator
    cmp al, 0
    je .print_done

    ; Wait for the transmit buffer to be empty
    mov dx, PORT + 5
.wait_transmit:
    in al, dx
    test al, 0x20
    jz .wait_transmit

    ; Send the character
    mov al, [esi]
    mov dx, PORT
    out dx, al

    inc esi
    jmp .print_loop

.print_done:
    ret