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
;   00002000 - 000027ff     2,048 bytes     Kernel-defined ISR table
;   00002800 - 00002fff     2,048 bytes     ISR thunk table
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

; Segment selectors
Segment.Kernel.Data equ     0x08
Segment.Kernel.Code equ     0x10
Segment.User.Data   equ     0x18
Segment.User.Code   equ     0x20
Segment.TSS         equ     0x28


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
; times into the ISR thunk table. The purpose of the thunk code is to push the
; interrupt number onto the stack before calling a general-purpose interrupt
; dispatcher. This is necessary because the interrupt number would otherwise
; be unavailable to the interrupt service routine. The thunk must be 8 bytes
; in length.
;-----------------------------------------------------------------------------

align 8
ISR.Thunk.Template:
    db      0x90                ; "nop" for padding and alignment
    db      0x6a, 0             ; "push <interrupt>" placeholder
    db      0xe9, 0, 0, 0, 0    ; "jmp <dispatcher>" placeholder

ISR.Thunk.Size   equ     ($ - ISR.Thunk.Template)


;-----------------------------------------------------------------------------
; ISR.Dispatcher
;
; A general-purpose ISR dispatch routine that all ISR thunks jump to when an
; interrupt arrives. The dispatcher receives from the stack an interrupt
; number, which it uses the to look up a kernel-defined ISR. If a valid ISR is
; found, the dispatcher calls it with a pointer to an interrupt context, which
; contains the contents on all general-purpose CPU registers, the interrupt
; number, and the error code if any.
;-----------------------------------------------------------------------------
ISR.Dispatcher:

    ; Push a dummy error code.
    push    0

    ; Preserve the first two general-purpose registers.
    push    r15
    push    r14

    .specialEntry:          ; Entry for ISR.Dispatcher.Special

        ; Preserve the rest of the general-purpose registers.
        push    r13
        push    r12
        push    r11
        push    r10
        push    r9
        push    r8
        push    rbp
        push    rdi
        push    rsi
        push    rdx
        push    rcx
        push    rbx
        push    rax

        ; Preserve the MXCSR register.
        sub     rsp,    8
        stmxcsr [rsp]

    .lookup:

        ; Look up the kernel-defined ISR in the table.
        mov     rax,    [rsp + 8 * 17]              ; rax=interrupt number
        mov     rax,    [Mem.ISR.Table + 8 * rax]   ; rax=ISR address

        ; If there is no ISR, then we're done.
        cmp     rax,    0
        je      .done

    .dispatch:

        ; The System V ABI requires the direction flag to be cleared on
        ; function entry.
        cld

        ; The interrupt context is on the stack, so pass the ISR a pointer to
        ; the stack as the first parameter.
        lea     rdi,    [rsp + 8]   ; skip the MXCSR register.

        ; Call the ISR.
        call    rax

    .done:

        ; Restore the MXCSR register.
        ldmxcsr [rsp]
        add     rsp,    8

        ; Restore general-purpose registers.
        pop     rax
        pop     rbx
        pop     rcx
        pop     rdx
        pop     rsi
        pop     rdi
        pop     rbp
        pop     r8
        pop     r9
        pop     r10
        pop     r11
        pop     r12
        pop     r13
        pop     r14
        pop     r15
        add     rsp,    16      ; Chop error code and interrupt #

        iretq


;-----------------------------------------------------------------------------
; ISR.Dispatcher.Special
;
; A special dispatcher is used for exceptions 8 and 10 through 14. The CPU
; pushes an error code onto the stack before calling these exceptions'
; interrupt handlers. So, to be compatible with the normal ISR.Dispatcher
; routine, we need to swap the places of the thunk-placed interrupt number and
; the error code on the stack.
;-----------------------------------------------------------------------------
ISR.Dispatcher.Special:

    ; First preserve r14 and r15.
    push    r15
    push    r14

    ; Use r14 and r15 to swap the interrupt number and error code entries on
    ; the stack.
    mov     r14,            [rsp + 8 * 2]    ; interrupt
    mov     r15,            [rsp + 8 * 3]    ; error code
    mov     [rsp + 8 * 2],  r15
    mov     [rsp + 8 * 3],  r14

    ; Jump directly to the normal dispatcher, but just beyond the step
    ; that inserts a dummy error code and preserves r14 and r15.
    jmp     ISR.Dispatcher.specialEntry


;-----------------------------------------------------------------------------
; @function     interrupts_init
; @brief        Initialize all interrupt tables.
; @details      Initialize a table of interrupt service routine thunks, one
;               for each of the 256 possible interrupts. Then set up the
;               interrupt descriptor table (IDT) to point to each of the
;               thunks.
; @killedregs   rax, rcx, rsi, rdi, r8
;-----------------------------------------------------------------------------
interrupts_init:

    ; CPU exception constants used by this code.
    .Exception.NMI  equ     0x02
    .Exception.DF   equ     0x08
    .Exception.TS   equ     0x0a
    .Exception.NP   equ     0x0b
    .Exception.SS   equ     0x0c
    .Exception.GP   equ     0x0d
    .Exception.PF   equ     0x0e
    .Exception.MC   equ     0x12

    ; Certain CPU exceptions push an error code onto the stack before calling
    ; the interrupt trap. We need to use a special dispatcher for these
    ; exceptions so it doesn't push an extra dummy error code onto the stack
    ; before calling the ISR.
    .PushesError    equ     (1 << .Exception.DF) | \
                            (1 << .Exception.TS) | \
                            (1 << .Exception.NP) | \
                            (1 << .Exception.SS) | \
                            (1 << .Exception.GP) | \
                            (1 << .Exception.PF)

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

        ; Initialize target pointer and interrupt counter.
        mov     rdi,    Mem.ISR.Thunks  ; rdi = thunk table offset
        xor     rcx,    rcx             ; rcx = interrupt number

    .installThunk:

        ; Copy the ISR thunk template to the current thunk entry.
        mov     rsi,    ISR.Thunk.Template
        movsq       ; assumes thunk is 8 bytes.

        ; Replace "push 0" with "push X", where X is the current interrupt
        ; number.
        mov     byte [rdi - ISR.Thunk.Size + 2],  cl

        ; By default, all thunks jump to ISR.Dispatcher.
        mov     r8,     ISR.Dispatcher

        ; Do we need to replace the dispatcher with the special dispatcher?

        ; If the current thunk is for an interrupt greater than 14, it will
        ; not use the special dispatcher, so jump ahead.
        cmp     rcx,    14
        ja      .updateDispatcher

        ; If the current thunk isn't for one of the exceptions that pushes an
        ; error code onto the stack, then jump ahead. (Use a bit mask to
        ; perform the check quickly.)
        mov     eax,    1
        shl     eax,    cl
        test    eax,    .PushesError
        jz      .updateDispatcher

        ; Use the special dispatcher for exceptions that push an error code
        ; onto the stack.
        mov     r8,     ISR.Dispatcher.Special

    .updateDispatcher:

        ; Replace "jmp 0" with "jmp <dispatcher offset>".
        sub     r8,                                 rdi
        mov     dword [rdi - ISR.Thunk.Size + 4],   r8d

        ; Advance to the next interrupt, and stop when we hit 256.
        inc     ecx
        cmp     ecx,    256
        jne     .installThunk

    ;-------------------------------------------------------------------------
    ; Initialize the interrupt descriptor table (IDT)
    ;-------------------------------------------------------------------------
    .setupIDT:

        ; Zero out the entire IDT first.
        xor     rax,    rax
        mov     rcx,    Mem.IDT.Size / 8
        mov     edi,    Mem.IDT
        rep     stosq

        ; Initialize pointers to transfer ISR thunk addresses into the IDT.
        mov     rsi,    Mem.ISR.Thunks      ; rsi = thunk table offset
        mov     rdi,    Mem.IDT             ; rdi = IDT offset
        xor     rcx,    rcx                 ; rcx = interrupt number

    .installDescriptor:

        ; Copy thunk table offset into r8 so we can modify it.
        mov     r8,     rsi

        ; Write the ISR thunk address bits [0:15].
        inc     r8      ; skip the nop
        mov     word [rdi + IDT.Descriptor.OffsetLo],   r8w

        ; Write the ISR thunk address bits [16:31].
        shr     r8,     16
        mov     word [rdi + IDT.Descriptor.OffsetMid],  r8w

        ; Write the ISR thunk address bits [32:63].
        shr     r8,     16
        mov     dword [rdi + IDT.Descriptor.OffsetHi],  r8d

        ; Write the kernel code segment selector.
        mov     r8w,    Segment.Kernel.Code
        mov     word [rdi + IDT.Descriptor.Segment],    r8w

        ; CPU exceptions are interrupts with vector numbers less than 32. For
        ; these interrupts, use an interrupt gate. For all others, use a trap
        ; gate.
        cmp     cl,     31
        ja      .useTrap

    .useInterrupt:

        ; Write the flags (IST=0, Type=interrupt, DPL=0, P=1)
        mov     word [rdi + IDT.Descriptor.Flags], 1000111000000000b
        jmp     .nextDescriptor

    .useTrap:

        ; Write the flags (IST=0, Type=trap, DPL=0, P=1)
        mov     word [rdi + IDT.Descriptor.Flags], 1000111100000000b

    .nextDescriptor:

        ; Advance to next thunk and IDT entry.
        add     rsi,    ISR.Thunk.Size
        add     rdi,    IDT.Descriptor_size

        ; Stop when we hit interrupt 256.
        inc     ecx
        cmp     ecx,    256
        jne     .installDescriptor

    .replaceSpecialStacks:

        ; Three exceptions (DF, MC, and NMI) require special stacks. So
        ; replace their IDT entries' flags with values that use different
        ; interrupt stack table (IST) settings.

        ; NMI exception (IST=1, Type=interrupt, DPL=0, P=1)
        mov     rdi,        Mem.IDT + \
                                IDT.Descriptor_size * .Exception.NMI + \
                                IDT.Descriptor.Flags
        mov     word [rdi], 1000111000000001b

        ; Double-fault exception (IST=2, Type=interrupt, DPL=0, P=1)
        mov     rdi,        Mem.IDT + \
                                IDT.Descriptor_size * .Exception.DF + \
                                IDT.Descriptor.Flags
        mov     word [rdi], 1000111000000010b

        ; Machine-check exception (IST=3, Type=interrupt, DPL=0, P=1)
        mov     rdi,        Mem.IDT + \
                                IDT.Descriptor_size * .Exception.MC + \
                                IDT.Descriptor.Flags
        mov     word [rdi], 1000111000000011b

    .loadIDT:

        ; Install the IDT
        lidt    [IDT.Pointer]

        ; Restore original interrupt flag setting.
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

    ; Restore original interrupt flag setting.
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

        ; Compute the mask (1 << IRQ).
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

        ; Compute the mask (1 << IRQ).
        mov     edx,    1
        shl     edx,    cl

        ; Read the current mask.
        in      al,     0xa1

        ; Set the IRQ bit and update the mask.
        or      al,     dl
        out     0xa1,   al

        ret
