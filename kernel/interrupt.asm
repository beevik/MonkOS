;=============================================================================
; @file     interrupt.asm
; @brief    Interrupt descriptor table and service routine functionality.
;
; Copyright 2016 Brett Vickers.
; Use of this source code is governed by a BSD-style license that can
; be found in the MonkOS LICENSE file.
;=============================================================================

bits 64

section .text

    global interrupts_init
    global isr_set
    global irq_enable
    global irq_disable


;-----------------------------------------------------------------------------
; Interrupt memory layout
;
;   00001000 - 00001fff     4,096 bytes     Interrupt descriptor table (IDT)
;   00002000 - 00002800     2,048 bytes     Kernel-defined ISR table
;   00002800 - 000038ff     4,352 bytes     ISR thunk table
;
; The IDT contains 256 interrupt descriptors, each of which points at one of
; the internet service routine (ISR) thunks. The thunks prepare a jump to a
; general-purpose ISR dispatcher, which calls the appropriate ISR from the
; kernel-defined ISR table.
;-----------------------------------------------------------------------------

; IDT memory range
Mem.IDT             equ     0x00001000
Mem.IDT.Size        equ     IDT.Descriptor_size * 256

; Kernel ISR table
Mem.ISR.Table       equ     0x00002000
Mem.ISR.Table.Size  equ     8 * 256         ; Pointer per interrupt

; ISR thunks
Mem.ISR.Thunks      equ     0x00002800
Mem.ISR.Thunks.Size equ     17 * 256        ; 17 bytes of code per interrupt


;-----------------------------------------------------------------------------
; IDT descriptor
;
; Each 64-bit IDT descriptor is a 16-byte structure organized as follows:
;
;     31                   16 15                    0
;    +-----------------------+-----------------------+
;    |        Segment        |         Offset        |
;    |       Selector        |          0:15         |
;    +-----------------------+-----------------------+
;    |        Offset         |         Flags         |
;    |        31:16          |                       |
;    +-----------------------+-----------------------+
;    |                    Offset                     |
;    |                     63:32                     |
;    +-----------------------+-----------------------+
;    |                   Reserved                    |
;    |                                               |
;    +-----------------------+-----------------------+
;
;         Bits
;       [0:15]      Interrupt handler offset bits [0:15]
;      [16:31]      Code segment selector
;      [32:34]      IST (Interrupt stack table)
;      [35:39]      Must be 0
;      [40:43]      Type (1110 = interrupt, 1111 = trap)
;          44       Must be 0
;      [45:46]      DPL (Descriptor privilege level) 0 = highest
;          47       P (Present)
;      [48:63]      Interrupt handler offset bits [16:31]
;      [64:95]      Interrupt handler offset bits [32:63]
;      [96:127]     Reserved
;
;-----------------------------------------------------------------------------
struc IDT.Descriptor

    .OffsetLo       resw    1
    .Segment        resw    1
    .Flags          resw    1
    .OffsetMid      resw    1
    .OffsetHi       resd    1
    .Reserved       resd    1

endstruc

; IDT pointer, which is loaded by the lidt instruction.
align 8
IDT.Pointer:
    dw  Mem.IDT.Size - 1    ; Limit = offset of last byte in table
    dq  Mem.IDT             ; Address of table copy


;-----------------------------------------------------------------------------
; ISR.Thunk.Template
;
; During initialization, the ISR thunk template is copied and modified 256
; times into the ISR thunk table. The purpose of the thunk is to present a
; common interface to the general-purpose ISR dispatcher. This is necessary
; because some interrupts push an extra error code onto the stack before their
; handlers are called, and others don't. It's also necessary so that we can
; push the interrupt number as a parameter to the general-purpose dispatcher.
;-----------------------------------------------------------------------------

align 8
ISR.Thunk.Template:
    push    0       ; push dummy error code (instruction skipped for 8,10-14).
    push    0       ; push the interrupt number. 0 is modified during init.
    push    rax     ; preserve rax here instead of in dispatcher.
    mov     rax,    ISR.Dispatcher
    jmp     rax     ; do absolute jump to ISR.Dispatcher.

ISR.Thunk.Size   equ     ($ - ISR.Thunk.Template)


;-----------------------------------------------------------------------------
; ISR.Dispatcher
;
; A general-purpose ISR dispatch routine that all ISR thunks jump to when an
; interrupt arrives. The dispatcher receives an error code and interrupt
; number on the stack (as well as the contents of the rax register). It then
; uses the interrupt number to look up a kernel-defined ISR, which it then
; calls with the interrupt number and error code as parameters. All registers
; are preserved and restored around the ISR call.
;-----------------------------------------------------------------------------
ISR.Dispatcher:

    ; Preserve registers.  rax was preserved just before the jmp from the
    ; thunk.
    push    rbx
    push    rcx
    push    rdx
    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15
    push    rsi
    push    rdi
    push    rbp

    ; Grab the interrupt number and error code.
    xor     rax,    rax
    mov     al,     [esp + 15 * 8]      ; al = interrupt number
    mov     rdi,    rax                 ; rdi = interrupt number
    mov     rsi,    [esp + 16 * 8]      ; rsi = error code

    ; Look up the kernel-defined ISR in the table.
    mov     rax,    [Mem.ISR.Table + rdi * 8]

    ; If there is no ISR, then we're done.
    cmp     rax,    0
    je      .done

    .found:

        ; The System V ABI requires the direction flag to be cleared on
        ; function entry.
        cld

        ; Call the ISR with the interrupt (rdi) and error code (rsi).
        call    rax

    .done:

        ; Restore registers
        pop     rbp
        pop     rdi
        pop     rsi
        pop     r15
        pop     r14
        pop     r13
        pop     r12
        pop     r11
        pop     r10
        pop     r9
        pop     r8
        pop     rdx
        pop     rcx
        pop     rbx
        pop     rax
        add     rsp,    16      ; Chop error code and interrupt #

        iretq


;-----------------------------------------------------------------------------
; @function     interrupts_init
; @brief        Initialize all interrupt tables.
; @details      Initialize a table of interrupt service routine thunks, one
;               for each of the 256 possible interrupts. Then set up the
;               interrupt descriptor table (IDT) to point to each of the
;               thunks. For interrupts 8 and 10-14, perform some offsetting
;               in the IDT descriptor to skip one of the thunk's push
;               instructions.
; @killedregs   rax, rcx, rdx, rsi, rdi, r8
;-----------------------------------------------------------------------------
interrupts_init:

    ; Preserve flags before disabling interrupts.
    pushf

    ; Interrupts must be disabled before mucking with interrupt tables.
    cli

    ;-------------------------------------------------------------------------
    ; Initialize the ISR thunk table
    ;-------------------------------------------------------------------------
    .setupThunks:

        ; Duplicate the ISR thunk template 256 times into the ISR thunk table.
        ; As the entry is duplicated, modify the interrupt number it pushes on
        ; the stack.
        mov     rdi,    Mem.ISR.Thunks
        xor     edx,    edx         ; edx = interrupt number

    .installThunk:

        ; Copy the ISR template to the current thunk entry.
        mov     rsi,    ISR.Thunk.Template
        mov     ecx,    ISR.Thunk.Size
        rep     movsb

        ; Modify the second "push 0" with "push X", where X is the
        ; current interrupt number.
        mov     byte [rdi - ISR.Thunk.Size + 3],  dl

        ; Advance to the next interrupt, and stop when we hit 256.
        inc     edx
        cmp     edx,    256
        jne     .installThunk

    ;-------------------------------------------------------------------------
    ; Initialize the interrupt descriptor table (IDT)
    ;-------------------------------------------------------------------------
    .setupIDT:

        ; Create a bitmask representing the interrupts (exceptions) that push
        ; an error code on the stack before their handlers are called. We'll
        ; need to treat these thunks specially so that they don't push an
        ; extra dummy error code onto the stack.
        .PushesError    equ      (1 << 8) | (1 << 10) | (1 << 11) | \
                                (1 << 12) | (1 << 13) | (1 << 14)

        ; Zero out the entire IDT first.
        xor     rax,    rax
        mov     rcx,    Mem.IDT.Size / 8
        mov     edi,    Mem.IDT
        rep     stosq

        ; Initialize pointers to transfer ISR thunk addresses into the IDT.
        mov     rsi,    Mem.ISR.Thunks      ; rsi = thunk table offset
        mov     rdi,    Mem.IDT             ; rdi = IDT offset
        xor     rdx,    rdx                 ; rdx = interrupt number

    .installDescriptor:

        ; Copy thunk table offset into r8 so we can modify it.
        mov     r8,     rsi

        ; If the current interrupt is greater than 14, it doesn't push an
        ; error, so skip ahead.
        cmp     rdx,    14
        ja      .storeDescriptor

        ; If the current interrupt is not 8 or 10 through 14, skip ahead. Use
        ; the bitmask for a fast check.
        mov     cl,     dl
        mov     eax,    1
        shl     eax,    cl
        test    eax,    .PushesError
        jz      .storeDescriptor

        ; Since the current interrupt pushes an error code, offset the thunk
        ; address by 2. This causes the "push 0" dummy error code to be
        ; skipped.
        add     r8,     2

    .storeDescriptor:

        ; Write the ISR thunk address bits [0:15].
        mov     rax,    r8
        mov     word [rdi + IDT.Descriptor.OffsetLo],   ax

        ; Write the ISR thunk address bits [16:31].
        shr     rax,    16
        mov     word [rdi + IDT.Descriptor.OffsetMid],  ax

        ; Write the ISR thunk address bits [32:63].
        shr     rax,    16
        mov     dword [rdi + IDT.Descriptor.OffsetHi],  eax

        ; Write the code segment selector.
        mov     rax,    0x08
        mov     word [rdi + IDT.Descriptor.Segment],    ax

        ; Use an interrupt gate for interrupts less than or equal to 31. Use a
        ; trap gate for interrupts greater than 31.
        cmp     rdx,    31
        ja      .useTrap

    .useInterrupt:

        ; Write the flags (IST=0, Type=interrupt, DPL=0, P=1)
        mov     word [rdi + IDT.Descriptor.Flags], 1000111000000000b
        jmp     .nextDescriptor

    .useTrap:

        ; Write the flags (IST=0, Type=trap, DPL=0, P=1)
        mov     word [rdi + IDT.Descriptor.Flags], 1000111100000000b

    .nextDescriptor:

        ; Advance to next interrupt, ISR thunk and IDT entry.
        inc     edx
        add     rsi,    ISR.Thunk.Size
        add     rdi,    IDT.Descriptor_size

        ; Are we done?
        cmp     edx,    256
        jne     .installDescriptor

    .loadIDT:

        ; Install the IDT
        lidt    [IDT.Pointer]

        ; Restore interrupt flag.
        popf

        ret


;-----------------------------------------------------------------------------
; @function     isr_set
; @brief        Set a kernel-defined interrupt service routine for the given
;               interrupt number.
; @details      Interrupts should be disabled while setting these handlers.
;               To disable an ISR, set its handler to null.
; @reg[in]      rdi     Interrupt number
; @reg[in]      rsi     ISR function address
;-----------------------------------------------------------------------------
isr_set:

    ; Preserve flags before disabling interrupts.
    pushf

    ; Temporarily disable interrupts while updating the ISR table.
    cli

    ; Multiply the interrupt number by 8 to get its ISR table offset.
    shl     rdi,    3
    add     rdi,    Mem.ISR.Table

    ; Store the interrupt service routine.
    mov     [rdi],  rsi

    ; Restore interrupt flag.
    popf

    ret


;-----------------------------------------------------------------------------
; @function     irq_enable
; @brief        Tell the PIC to enable a hardware interrupt.
; @reg[in]      rdi     IRQ number.
; @killedregs   rax, rcx, rdx
;-----------------------------------------------------------------------------
irq_enable:

    ; Move IRQ into cl.
    mov     rcx,    rdi

    ; Determine which PIC to update (<8 = master, else slave).
    cmp     cl,     8
    jae     .slave

    .master:

        ; Compute the mask ~(1 << IRQ).
        mov     edx,    1
        shl     edx,    cl
        not     edx

        ; Read the current mask.
        in      al,     0x21

        ; Clear the IRQ bit and update the mask.
        and     al,     dl
        out     0x21,   al

        ret

    .slave:

        ; Recursively enable master IRQ2, or else slave IRQs will not work.
        mov     rdi,    2
        call    irq_enable

        ; Subtract 8 from the IRQ.
        sub     cl,     8

        ; Compute the mask ~(1 << IRQ).
        mov     edx,    1
        shl     edx,    cl
        not     edx

        ; Read the current mask.
        in      al,     0xa1

        ; Clear the IRQ bit and update the mask.
        and     al,     dl
        out     0xa1,   al

        ret


;-----------------------------------------------------------------------------
; @function     irq_disable
; @brief        Tell the PIC to disable a hardware interrupt.
; @reg[in]      rdi     IRQ number.
; @killedregs   rax, rcx, rdx
;-----------------------------------------------------------------------------
irq_disable:

    ; Move IRQ into cl.
    mov     rcx,    rdi

    ; Determine which PIC to update (<8 = master, else slave).
    cmp     cl,     8
    jae     .slave

    .master:

        ; Compute the mask ~(1 << IRQ).
        mov     edx,    1
        shl     edx,    cl

        ; Read the current mask.
        in      al,     0x21

        ; Set the IRQ bit and update the mask.
        or      al,     dl
        out     0x21,   al

        ret

    .slave:

        ; Subtract 8 from the IRQ.
        sub     cl,     8

        ; Compute the mask ~(1 << IRQ).
        mov     edx,    1
        shl     edx,    cl

        ; Read the current mask.
        in      al,     0xa1

        ; Set the IRQ bit and update the mask.
        or      al,     dl
        out     0xa1,   al

        ret
