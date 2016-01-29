;=============================================================================
; @file     io.asm
; @brief    Port I/O routines.
;
; Copyright 2016 Brett Vickers.
; Use of this source code is governed by a BSD-style license that can
; be found in the MonkOS LICENSE file.
;=============================================================================

bits 64

section .text

    global  io_inb
    global  io_outb

;-----------------------------------------------------------------------------
; @function     io_inb
; @brief        Retrieve a byte value from an input port.
; @reg[in]      rdi     Port number (0-65535).
; @reg[out]     rax     Byte value read from the port.
;-----------------------------------------------------------------------------
io_inb:

    ; Ports must be read through the dx and al registers.
    mov     rdx,    rdi

    ; Read the input port into al.
    xor     rax,    rax
    in      al,     dx

    ret


;-----------------------------------------------------------------------------
; @function     io_outb
; @brief        Write a byte value to an output port.
; @reg[in]      rdi     Port number (0-65535).
; @reg[in]      rsi     Byte value to write to the port.
;-----------------------------------------------------------------------------
io_outb:

    ; Ports must be written through the dx and al registers.
    mov     rdx,    rdi
    mov     rax,    rsi

    ; Write the byte value to the port.
    out     dx,     al

    ret
