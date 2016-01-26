;=============================================================================
; @file gdt.inc
;
; Global Descrtiptor Table structure definitions for 32-bit and 64-bit
; modes.
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
; register values. See loader_iso.asm for the descriptors referenced by
; these selectors.
GDT32.Selector.Code32       equ     0x08    ; 32-bit protected mode (code)
GDT32.Selector.Data32       equ     0x10    ; 32-bit protected mode (data)
GDT32.Selector.Code16       equ     0x18    ; 16-bit protected mode (code)
GDT32.Selector.Data16       equ     0x20    ; 16-bit protected mode (data)

GDT64.Selector.Code         equ     0x08    ; 64-bit long mode (code)
GDT64.Selector.Data         equ     0x10    ; 64-bit long mode (data)


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
struc GDT

    .LimitLow:              resw    1
    .BaseLow:               resw    1
    .BaseMiddle:            resb    1
    .Access:                resb    1
    .LimitHighFlags:        resb    1   ; LimitHigh (4 bits) + Flags (4 bits)
    .BaseHigh:              resb    1

endstruc

%endif ; __MONK_BOOT_GDT_INC__
