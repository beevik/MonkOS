;=============================================================================
; @file     memsetw.asm
; @brief    Fill a region of memory with a single word value.
;
; Copyright 2016 Brett Vickers.
; Use of this source code is governed by a BSD-style license that can
; be found in the MonkOS LICENSE file.
;=============================================================================

bits 64

section .text

    global memsetw


;-----------------------------------------------------------------------------
; @function     memsetw
; @brief        Fill a region of memory with a single 16-bit word value.
; @reg[in]      rdi     Address of the destination memory area.
; @reg[in]      rsi     Value of the word used to fill memory.
; @reg[in]      rdx     Number of words to set.
; @reg[out]     rax     Destination address.
; @killedregs   r8, rcx
;-----------------------------------------------------------------------------
memsetw:

    ; Preserve the original destination address.
    mov     r8,     rdi

    ; The value to store is the second parameter (rsi).
    mov     rax,    rsi

    ; Do a byte-by-byte store.
    mov     rcx,    rdx
    rep     stosw

    ; Return the original destination address.
    mov     rax,    r8
    ret
