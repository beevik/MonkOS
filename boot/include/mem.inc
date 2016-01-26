;=============================================================================
; @file mem.inc
;
; Memory layout constants and macros used by boot loader code.
;
; Copyright 2016 Brett Vickers.
; Use of this source code is governed by a BSD-style license that can
; be found in the LICENSE file.
;=============================================================================

%ifndef __MONK_BOOT_MEM_INC__
%define __MONK_BOOT_MEM_INC__

;=============================================================================
; Memory layout
;=============================================================================

Mem.Globals                     equ     0x00000500
Mem.GDT                         equ     0x00000700
Mem.Sector.Buffer               equ     0x00000800
Mem.Stack.Bottom                equ     0x00001000
Mem.Stack.Top                   equ     0x00007c00
Mem.Loader1                     equ     0x00007c00
Mem.Loader2                     equ     0x00008000
Mem.PageTable                   equ     0x00010000
Mem.PageTable.PML4T             equ     0x00010000
Mem.PageTable.PDPT              equ     0x00011000
Mem.PageTable.PDT               equ     0x00012000
Mem.PageTable.PT0               equ     0x00013000  ; maps  0: 1 MiB
Mem.PageTable.PT1               equ     0x00014000  ; maps  2: 3 MiB
Mem.PageTable.PT2               equ     0x00015000  ; maps  4: 5 MiB
Mem.PageTable.PT3               equ     0x00016000  ; maps  6: 7 MiB
Mem.PageTable.PT4               equ     0x00017000  ; maps  8: 9 MiB
Mem.PageTable.End               equ     0x00018000
Mem.Stack32.Temp.Bottom         equ     0x0006f000
Mem.Stack32.Temp.Top            equ     0x00070000
Mem.Kernel.LoadBuffer           equ     0x00070000
Mem.Kernel.Stack                equ     0x00090000
Mem.Kernel.Image                equ     0x00100000
Mem.Kernel.Code                 equ     0x00101000

; Layout region sizes
Mem.Sector.Buffer.Size          equ     0x00000800
Mem.Kernel.LoadBuffer.Size      equ     0x00010000

; Real mode segment addresses
Mem.Loader1.Segment             equ     Mem.Loader1 >> 4
Mem.Loader2.Segment             equ     Mem.Loader2 >> 4
Mem.Kernel.LoadBuffer.Segment   equ     Mem.Kernel.LoadBuffer >> 4

%endif ; __MONK_BOOT_MEM_INC__
