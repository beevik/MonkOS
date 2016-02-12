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

Mem.BIOS.IVT                        equ     0x00000000
Mem.BIOS.Data                       equ     0x00000400
Mem.GDT                             equ     0x00000100
Mem.TSS64                           equ     0x00000200
Mem.Globals                         equ     0x00000500
Mem.Sector.Buffer                   equ     0x00000800
Mem.Stack.Bottom                    equ     0x00001000
Mem.Stack.Top                       equ     0x00007c00
Mem.Loader1                         equ     0x00007c00
Mem.Loader2                         equ     0x00008000
Mem.PageTable                       equ     0x00010000
Mem.PageTable.PML4T                 equ     0x00010000
Mem.PageTable.PDPT                  equ     0x00011000
Mem.PageTable.PDT                   equ     0x00012000  ; maps first 10MiB
Mem.PageTable.PT                    equ     0x00013000  ; maps first 2MiB
Mem.PageTable.End                   equ     0x00014000
Mem.Layout                          equ     0x00018000  ; BIOS-derived layout
Mem.Stack32.Temp.Bottom             equ     0x0006f000
Mem.Stack32.Temp.Top                equ     0x00070000
Mem.Kernel.LoadBuffer               equ     0x00070000
Mem.Kernel.Stack.NMI.Bottom         equ     0x0008a000  ; NMI stack
Mem.Kernel.Stack.NMI.Top            equ     0x0008c000
Mem.Kernel.Stack.DF.Bottom          equ     0x0008c000  ; double-fault stack
Mem.Kernel.Stack.DF.Top             equ     0x0008e000
Mem.Kernel.Stack.MC.Bottom          equ     0x0008e000  ; machine-check stack
Mem.Kernel.Stack.MC.Top             equ     0x00090000
Mem.BIOS.EBDA                       equ     0x0009e000
Mem.Video                           equ     0x000a0000
Mem.Kernel.Stack.Interrupt.Bottom   equ     0x00100000  ; PL-change intr stack
Mem.Kernel.Stack.Interrupt.Top      equ     0x001ff000
Mem.Kernel.Stack.Bottom             equ     0x00200000  ; main kernel stack
Mem.Kernel.Stack.Top                equ     0x00300000
Mem.Kernel.Image                    equ     0x00300000
Mem.Kernel.Code                     equ     0x00301000

; Layout region sizes
Mem.BIOS.IVT.Size                   equ     0x00000400
Mem.BIOS.Data.Size                  equ     0x00000100
Mem.Loader1.Size                    equ     0x00000200
Mem.Loader2.Size                    equ     0x00008000
Mem.Sector.Buffer.Size              equ     0x00000800
Mem.Layout.Size                     equ     0x00004000
Mem.Kernel.LoadBuffer.Size          equ     0x00010000

; Real mode segment addresses
Mem.Loader1.Segment                 equ     Mem.Loader1 >> 4
Mem.Loader2.Segment                 equ     Mem.Loader2 >> 4

%endif ; __MONK_BOOT_MEM_INC__
