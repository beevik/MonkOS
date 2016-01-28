;=============================================================================
; @file strings.asm
;
; String and memory manipulation functions.
;
; All functions use the 64-bit calling convention of the System V ABI.
;
; Copyright 2016 Brett Vickers.
; Use of this source code is governed by a BSD-style license that can
; be found in the MonkOS LICENSE file.
;=============================================================================

bits 64

section .text

    global memcopy
    global memmove
    global memset
    global memzero


;=============================================================================
; memcopy
;
; Copy a number of bytes from the source address to the destination address.
; If the memory areas overlap, this function's behavior is undefined.
;
; Input registers:
;   rdi     destination address
;   rsi     source address
;   rdx     number of bytes to copy
;
; Return registers:
;   rax     destination address
;
; Killed registers:
;   rcx
;=============================================================================
memcopy:

    ; Clear the direction flag so copying is left-to-right.
    cld

    ; Preserve destination address because we have to return it.
    mov     rax,    rdi

    ; Do a byte-by-byte move. On modern x86 chips, there is not a significant
    ; performance difference between byte-wise and word-wise moves. In fact,
    ; sometimes movsb is faster. See instlatx64.atw.hu for benchmarks.
    mov     rcx,    rdx
    rep     movsb

    ret


;=============================================================================
; memmove
;
; Copy a number of bytes from the source address to the destination address,
; even if the memory areas overlap.
;
; Input registers:
;   rdi     destination address
;   rsi     source address
;   rdx     number of bytes to copy
;
; Return registers:
;   rax     destination address
;
; Killed registers:
;   rcx
;=============================================================================
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

        ; Clear the direction flag so copying is left-to-right.
        cld

        ; Do a byte-by-byte move.
        mov     rcx,    rdx
        rep     movsb

    .done:

        ret


;=============================================================================
; memset
;
; Set a number of bytes to a given character value starting at the
; destination address.
;
; Input registers:
;   rdi     destination address
;   rsi     character
;   rdx     number of bytes
;
; Return registers:
;   rax     destination address
;
; Killed registers:
;   r8, rcx
;=============================================================================
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


;=============================================================================
; memzero
;
; Set a number of bytes to zero starting at the destination address.
;
; Input registers:
;   rdi     destination address
;   rsi     number of bytes to zero
;
; Return registers:
;   rax     destination address
;
; Killed registers:
;   r8, rcx
;=============================================================================
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
