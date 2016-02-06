;=============================================================================
; @file     start.asm
; @brief    The kernel launcher.
; @details  The boot loader launches the kernel by jumping to _start.
;
; Copyright 2016 Brett Vickers.
; Use of this source code is governed by a BSD-style license that can
; be found in the MonkOS LICENSE file.
;=============================================================================

; The boot loader should have put us in 64-bit long mode.
bits 64

; Include boot loader's memory layout.
%include "../boot/include/mem.inc"

; Use a special section .start, which comes first in the linker.ld .text
; section. This way, the _start label will be given the lowest possible code
; address (0x00301000 in our case).
section .start
    global _start

    extern kmain        ; Exported by main.c
    extern memzero      ; Exported by strings.asm
    extern _BSS_START   ; Linker-generated symbol
    extern _BSS_SIZE    ; Linker-generated symbol

;-----------------------------------------------------------------------------
; @function     _start
; @brief        Kernel entry point, called by the boot loader.
;-----------------------------------------------------------------------------
_start:

    ; The System V ABI requires the direction flag to be clear on function
    ; entry.
    cld

    ; Zero out the stage-2 loader now that it's done running.
    mov     rdi,    Mem.Loader2
    mov     rsi,    Mem.Loader2.Size
    call    memzero

    ; Zero out the kernel's bss section.
    mov     rdi,    _BSS_START
    mov     rsi,    _BSS_SIZE
    call    memzero

    ; Call the kernel's main entry point. This function should never return.
    call    kmain

    ; If the function does return for some reason, hang the computer.
    .hang:
        cli
        hlt
        jmp     .hang
