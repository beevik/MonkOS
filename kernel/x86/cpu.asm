;=============================================================================
; @file     asm.asm
; @brief    x86 CPU-specific function implementations.
; @brief    x86 CPU assembly code helpers.
; @details  These implementations are for builds performed without inline
;           assembly.
;
; Copyright 2016 Brett Vickers.
; Use of this source code is governed by a BSD-style license that can
; be found in the MonkOS LICENSE file.
;=============================================================================

bits 64

section .text

    global cpuid
    global rdmsr
    global wrmsr
    global io_inb
    global io_outb
    global io_inw
    global io_outw
    global io_ind
    global io_outd
    global set_pagetable
    global invalidate_page
    global enable_interrupts
    global disable_interrupts
    global halt
    global invalid_opcode
    global fatal

;-----------------------------------------------------------------------------
; @function     cpuid
; @brief        Return the results of the CPUID instruction.
; @reg[in]      rdi     The cpuid group code.
; @reg[in]      rsi     pointer to a registers4_t struct.
;-----------------------------------------------------------------------------
cpuid:

    push    rbx

    mov     rax,    rdi
    cpuid

    mov     [rsi + 8 * 0],  rax
    mov     [rsi + 8 * 1],  rbx
    mov     [rsi + 8 * 2],  rcx
    mov     [rsi + 8 * 3],  rdx

    pop     rbx
    ret

;-----------------------------------------------------------------------------
; @function     rdmsr
; @brief        Read the model-specific register and return the result.
; @reg[in]      rdi     The MSR register id to read.
; @reg[out]     rax     The contents of the requested MSR.
;-----------------------------------------------------------------------------
rdmsr:

    mov     rcx,    rdi

    rdmsr

    shl     rdx,    32
    or      rax,    rdx
    ret


;-----------------------------------------------------------------------------
; @function     wrmsr
; @brief        Write to the model-specific register.
; @reg[in]      rdi     The MSR register id to write.
; @reg[in]      rsi     The value to write.
;-----------------------------------------------------------------------------
wrmsr:

    mov     ecx,    edi

    mov     rax,    rsi
    mov     rdx,    rax
    shr     rdx,    32

    wrmsr

    ret

;-----------------------------------------------------------------------------
; @function     io_inb
; @brief        Retrieve a byte value from an input port.
; @reg[in]      rdi     Port number (0-65535).
; @reg[out]     rax     Byte value read from the port.
;-----------------------------------------------------------------------------
io_inb:

    mov     dx,     di
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

    mov     dx,     di
    mov     ax,     si
    out     dx,     al
    ret

;-----------------------------------------------------------------------------
; @function     io_inw
; @brief        Retrieve a 16-bit word value from an input port.
; @reg[in]      rdi     Port number (0-65535).
; @reg[out]     rax     Word value read from the port.
;-----------------------------------------------------------------------------
io_inw:

    mov     dx,     di
    xor     rax,    rax
    in      ax,     dx
    ret

;-----------------------------------------------------------------------------
; @function     io_outw
; @brief        Write a 16-bit word value to an output port.
; @reg[in]      rdi     Port number (0-65535).
; @reg[in]      rsi     Word value to write to the port.
;-----------------------------------------------------------------------------
io_outw:

    mov     dx,     di
    mov     ax,     si
    out     dx,     ax
    ret

;-----------------------------------------------------------------------------
; @function     io_ind
; @brief        Retrieve a 32-bit dword value from an input port.
; @reg[in]      rdi     Port number (0-65535).
; @reg[out]     rax     Dword value read from the port.
;-----------------------------------------------------------------------------
io_ind:

    mov     dx,     di
    xor     rax,    rax
    in      eax,    dx
    ret

;-----------------------------------------------------------------------------
; @function     io_outd
; @brief        Write a 32-bit dword value to an output port.
; @reg[in]      rdi     Port number (0-65535).
; @reg[in]      rsi     Dord value to write to the port.
;-----------------------------------------------------------------------------
io_outd:

    mov     dx,     di
    mov     eax,    esi
    out     dx,     eax
    ret

;-----------------------------------------------------------------------------
; @function     set_pagetable
; @brief        Update the CPU's page table register.
; @reg[in]      rdi     The physical address containing the new pagetable.
;-----------------------------------------------------------------------------
set_pagetable:

    mov     cr3,    rdi
    ret

;-----------------------------------------------------------------------------
; @function     invalidate_page
; @brief        Invalidate the page containing a virtual address.
; @reg[in]      rdi     The virtual address of the page to invalidate.
;-----------------------------------------------------------------------------
invalidate_page:

    invlpg  [rdi]
    ret

;-----------------------------------------------------------------------------
; @function     enable_interrupts
; @brief        Enable interrupts.
;-----------------------------------------------------------------------------
enable_interrupts:

    sti
    ret

;-----------------------------------------------------------------------------
; @function     disable_interrupts
; @brief        Disable interrupts.
;-----------------------------------------------------------------------------
disable_interrupts:

    cli
    ret

;-----------------------------------------------------------------------------
; @function     halt
; @brief        Halt the CPU until an interrupt occurs.
;-----------------------------------------------------------------------------
halt:

    hlt
    ret

;-----------------------------------------------------------------------------
; @function     invalid_opcode
; @brief        Raise an invalid opcode exception.
;-----------------------------------------------------------------------------
invalid_opcode:

    int     6
    ret

;-----------------------------------------------------------------------------
; @function     fatal
; @brief        Raise a fatal interrupt that hangs the system.
;-----------------------------------------------------------------------------
fatal:

    int     0xff
    ret
