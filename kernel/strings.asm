;=============================================================================
; @file     strings.asm
; @brief    String and memory manipulation functions.
;
; Copyright 2016 Brett Vickers.
; Use of this source code is governed by a BSD-style license that can
; be found in the MonkOS LICENSE file.
;=============================================================================

bits 64

section .text

    global memcpy
    global memmove
    global memset
    global memsetw
    global memsetd
    global memzero


;-----------------------------------------------------------------------------
; @function     memcpy
; @brief        Copy bytes from one memory region to another.
; @details      If the memory regions overlap, this function's behavior
;               is undefined, and you should use memmove instead.
; @reg[in]      rdi     Address of the destination memory area.
; @reg[in]      rsi     Address of the source memory area.
; @reg[in]      rdx     Number of bytes to copy.
; @reg[out]     rax     Destination address.
; @regskilled   rcx
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


;-----------------------------------------------------------------------------
; @function     memmove
; @brief        Move bytes from one memory region to another, even if the
;               regions overlap.
; @reg[in]      rdi     Address of the destination memory area.
; @reg[in]      rsi     Address of the source memory area.
; @reg[in]      rdx     Number of bytes to copy.
; @reg[out]     rax     Destination address.
; @regskilled   rcx
;-----------------------------------------------------------------------------
memmove:

    ; Preserve destination address because we have to return it.
    mov     rax,    rdi

    ; If dest < src, we can always do a fast pointer-incrementing move.
    ; If dest == src, do nothing.
    cmp     rdi,    rsi
    je      .done
    jb      .fast

    ; If dest > src and there are no overlapping regions (dest >= src+num),
    ; we can still do a fast pointer-incrementing move.
    mov     rcx,    rsi
    add     rcx,    rdx
    cmp     rdi,    rcx
    jae     .fast

    ; If dest > src and dest < src+num, we have to do a right-to-left
    ; move to preserve overlapping data.
    .slow:

        ; Set the direction flag so copying is right-to-left.
        std

        ; Set the move count register.
        mov     rcx,    rdx

        ; Update pointers to the right-hand side (minus one).
        dec     rdx
        add     rsi,    rdx
        add     rdi,    rdx

        ; Do a byte-by-byte move.
        rep     movsb

        ; Reset the direction flag.
        cld

        ret

    .fast:

        ; Do a byte-by-byte move.
        mov     rcx,    rdx
        rep     movsb

    .done:

        ret


;-----------------------------------------------------------------------------
; @function     memset
; @brief        Fill a region of memory with a single byte value.
; @reg[in]      rdi     Address of the destination memory area.
; @reg[in]      rsi     Value of the byte used to fill memory.
; @reg[in]      rdx     Number of bytes to set.
; @reg[out]     rax     Destination address.
; @regskilled   r8, rcx
;-----------------------------------------------------------------------------
memset:

    ; Preserve the original destination address.
    mov     r8,     rdi

    ; The value to store is the second parameter (rsi).
    mov     rax,    rsi

    ; Do a byte-by-byte store.
    mov     rcx,    rdx
    rep     stosb

    ; Return the original destination address.
    mov     rax,    r8
    ret


;-----------------------------------------------------------------------------
; @function     memsetw
; @brief        Fill a region of memory with a single 16-bit word value.
; @reg[in]      rdi     Address of the destination memory area.
; @reg[in]      rsi     Value of the word used to fill memory.
; @reg[in]      rdx     Number of words to set.
; @reg[out]     rax     Destination address.
; @regskilled   r8, rcx
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


;-----------------------------------------------------------------------------
; @function     memsetd
; @brief        Fill a region of memory with a single 32-bit dword value.
; @reg[in]      rdi    Address of the destination memory area.
; @reg[in]      rsi    Value of the dword used to fill memory.
; @reg[in]      rdx    Number of dwords to set.
; @reg[out]     rax    Destination address.
; @regskilled   r8, rcx
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


;-----------------------------------------------------------------------------
; @function     memzero
; @brief        Fill a region of memory with zeroes.
; @reg[in]      rdi     Address of the destination memory area.
; @reg[in]      rsi     Number of bytes to set to zero.
; @reg[out]     rax     Destination address.
; @regskilled   r8, rcx
;-----------------------------------------------------------------------------
memzero:

    ; Preserve the original destination address.
    mov     r8,     rdi

    ; The value to store is zero.
    xor     rax,    rax

    ; Do a byte-by-byte store.
    mov     rcx,    rsi
    rep     stosb

    ; Return the original destination address.
    mov     rax,    r8
    ret
