;=============================================================================
; @file     memcpy.asm
; @brief    Copy bytes from one memory region to another.
;
; Copyright 2016 Brett Vickers.
; Use of this source code is governed by a BSD-style license that can
; be found in the MonkOS LICENSE file.
;=============================================================================

bits 64

section .text

    global memcpy


;-----------------------------------------------------------------------------
; @function     memcpy
; @brief        Copy bytes from one memory region to another.
; @details      If the memory regions overlap, this function's behavior
;               is undefined, and you should use memmove instead.
; @reg[in]      rdi     Address of the destination memory area.
; @reg[in]      rsi     Address of the source memory area.
; @reg[in]      rdx     Number of bytes to copy.
; @reg[out]     rax     Destination address.
; @killedregs   rcx
;-----------------------------------------------------------------------------
memcpy:

    ; Preserve destination address because we have to return it.
    mov     rax,    rdi

    ; Do a byte-by-byte move. On modern x86 chips, there is not a significant
    ; performance difference between byte-wise and word-wise moves. In fact,
    ; sometimes movsb is faster. See instlatx64.atw.hu for benchmarks.
    mov     rcx,    rdx
    rep     movsb

    ret
