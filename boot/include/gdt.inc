;=============================================================================
; @file gdt.inc
;
; Global Descriptor Table and Task State Segment structure definitions
; for 32-bit and 64-bit modes.
;
; Copyright 2016 Brett Vickers.
; Use of this source code is governed by a BSD-style license that can
; be found in the LICENSE file.
;=============================================================================

%ifndef __MONK_BOOT_GDT_INC__
%define __MONK_BOOT_GDT_INC__

;=============================================================================
; Constants
;=============================================================================

; GDT selectors, which should be used in protected mode to set segment
; register values. See loader_iso.asm for the descriptors referenced by these
; selectors.
GDT32.Selector.Code32       equ     0x08    ; 32-bit protected mode (code)
GDT32.Selector.Data32       equ     0x10    ; 32-bit protected mode (data)
GDT32.Selector.Code16       equ     0x18    ; 16-bit protected mode (code)
GDT32.Selector.Data16       equ     0x20    ; 16-bit protected mode (data)

GDT64.Selector.Kernel.Code  equ     0x08    ; 64-bit mode (kernel code)
GDT64.Selector.Kernel.Data  equ     0x10    ; 64-bit mode (kernel data)
GDT64.Selector.User.Data    equ     0x18    ; 64-bit mode (user data)
GDT64.Selector.User.Code    equ     0x20    ; 64-bit mode (user code)
GDT64.Selector.TSS          equ     0x28    ; 64-bit task state segment


;=============================================================================
; GDT
;
; Each GDT entry is an 8-byte structure organized as follows:
;
;     31                   16 15                    0
;    +-----------------------+-----------------------+
;    |         Base          |         Limit         |
;    |         0:15          |          0:15         |
;    +-------+-------+-------+-----------+-----------+
;    | Base  | Flags | Limit |  Access   |    Base   |
;    | 24:31 |       | 16:19 |           |   16:23   |
;    +-------+-------+-------+-----------+-----------+
;
;         Bits
;       [0:15]      Limit bits [0:15]
;      [16:31]      Base address bits [0:15]
;      [32:39]      Base address bits [16:23]
;          40       Access: AC (Accessed) bit
;          41       Access: RW (Readable/Writable) bit
;          42       Access: DC (Direction/Conforming) bit
;          43       Access: EX (Executable) bit
;          44       Access: must be 1
;      [45:46]      Access: Privilege ring (0 = highest, 3 = lowest)
;          47       Access: PR (Present) bit
;      [48:51]      Limit bits [16:19]
;          52       Flags: must be 0
;          53       Flags: LM (Long Mode)
;          54       Flags: SZ (Size) bit (0=16-bit, 1=32-bit)
;          55       Flags: GR (Granularity) (0=1B, 1=4KiB limit granularity)
;      [56:63]      Base address bits [24:31]
;
;=============================================================================
struc GDT.Descriptor

    .LimitLow:              resw    1
    .BaseLow:               resw    1
    .BaseMiddle:            resb    1
    .Access:                resb    1
    .LimitHighFlags:        resb    1   ; LimitHigh (4 bits) + Flags (4 bits)
    .BaseHigh:              resb    1

endstruc


;=============================================================================
; 64-bit TSS Descriptor
;
; Similar layout to GDT.Descriptor, but with two additional dwords, one of
; which is used to extend the base address.
;=============================================================================
struc TSS64.Descriptor

    .LimitLow:              resw    1
    .BaseLow:               resw    1
    .BaseMiddle:            resb    1
    .Access:                resb    1
    .LimitHighFlags:        resb    1   ; LimitHigh (4 bits) + Flags (4 bits)
    .BaseHigh:              resb    1
    .BaseHighest:           resd    1
    .Reserved:              resd    1

endstruc


;=============================================================================
; 64-bit TSS
;
; The 64-bit Task State Segment data structure.
;
; Contains information used during privilege mode changes.
;=============================================================================
struc TSS64

                            resd    1   ; Reserved
    .RSP0:                  resq    1   ; Stack pointer (priv 0)
    .RSP1:                  resq    1   ; ...
    .RSP2:                  resq    1
                            resq    1   ; Reserved
    .IST1:                  resq    1   ; Interrupt Stack Table pointer 1
    .IST2:                  resq    1   ; ...
    .IST3:                  resq    1
    .IST4:                  resq    1
    .IST5:                  resq    1
    .IST6:                  resq    1
    .IST7:                  resq    1
                            resq    1   ; Reserved
                            resw    1   ; Reserved
    .IOPB:                  resw    1   ; I/O permission bitmap offset

endstruc

%endif ; __MONK_BOOT_GDT_INC__
