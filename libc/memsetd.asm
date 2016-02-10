;=============================================================================
; @file     memsetd.asm
; @brief    Fill a region of memory with a single dword value.
;
; Copyright 2016 Brett Vickers.
; Use of this source code is governed by a BSD-style license that can
; be found in the MonkOS LICENSE file.
;=============================================================================

bits 64

section .text

    global memsetd


;-----------------------------------------------------------------------------
; @function     memsetd
; @brief        Fill a region of memory with a single 32-bit dword value.
; @reg[in]      rdi    Address of the destination memory area.
; @reg[in]      rsi    Value of the dword used to fill memory.
; @reg[in]      rdx    Number of dwords to set.
; @reg[out]     rax    Destination address.
; @killedregs   r8, rcx
;-----------------------------------------------------------------------------
memsetd:

    ; Preserve the original destination address.
    mov     r8,     rdi

    ; The value to store is the second parameter (rsi).
    mov     rax,    rsi

    ; Do a byte-by-byte store.
    mov     rcx,    rdx
    rep     stosd

    ; Return the original destination address.
    mov     rax,    r8
    ret
