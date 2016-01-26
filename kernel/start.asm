;=============================================================================
; @file start.asm
;
; The kernel launcher.
;
; The boot loader launches the kernel by jumping to _start.
;
; Copyright 2016 Brett Vickers.
; Use of this source code is governed by a BSD-style license that can
; be found in the MonkOS LICENSE file.
;=============================================================================

; The boot loader should have put us in 64-bit long mode.
bits 64

; Use a special section .start, which comes first in the linker.ld
; .text section. This way, the _start label will be given the lowest possible
; code address (0x00101000 in our case).
section .start
    global _start
    extern kmain        ; Exported by main.c
    extern _BSS_START   ; Linker-generated symbol
    extern _BSS_SIZE    ; Linker-generated symbol


;=============================================================================
; _start
;
; Kernel entry point, called by the boot loader.
;=============================================================================
_start:

    ; Zero out the kernel's bss section.
    call    ClearBSS

    ; Call the kernel's main entry point. This function should never
    ; return.
    call    kmain

    ; If the function does return for some reason, hang the computer.
    .hang:
        cli
        hlt
        jmp     .hang


;=============================================================================
; ClearBSS
;
; Zero out the entire bss section. Use the _BSS constants defined by the
; linker to discover the extents of the bss section.
;
; Killed registers:
;   None
;=============================================================================
ClearBSS:

    ; Preserve registers
    push    rdi
    push    rax
    push    rcx

    xor     eax,    eax

    ; Clear 8 bytes at a time.
    mov     rdi,    _BSS_START
    mov     rcx,    _BSS_SIZE
    shr     rcx,    3               ; _BSS_SIZE / 8
    rep     stosq

    ; Clear 1 byte at a time for the remainder.
    mov     rcx,    _BSS_SIZE
    and     rcx,    7               ; _BSS_SIZE modulo 8
    rep     stosb

    ; Restore registers
    pop     rcx
    pop     rax
    pop     rdi

    ret