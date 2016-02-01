;=============================================================================
; @file boot.asm
;
; A first-stage boot loader suitable for an El Torito ISO 9660 cdrom
; image.
;
; The first-stage boot loader is responsible for accessing the CDROM to locate
; and launch the second-stage boot loader (called LOADER.SYS). The second-
; stage boot loader may be up to 32KiB, which is larger than the tiny 512
; bytes afforded by the BIOS to the first-stage boot loader.
;
; To assemble this file, use raw binary mode with nasm, as follows:
;
;   nasm -f bin -o boot.sys boot_iso.asm
;
; Take the completed image file and copy it to the following directory after
; creating the necessary subdirectories:
;
;   ./iso/boot/boot.sys
;
; Make sure you have also added the second-stage loader binary in the
; following location:
;
;   ./iso/loader.sys
;
; Use genisoimage to create the bootable cdrom ISO file 'boot.iso':
;
;   genisoimage -R -J -c boot/bootcat -b boot/boot.sys \
;       -no-emul-boot -boot-load-size 4 -o boot.iso \
;       ./iso
;
; Copyright 2016 Brett Vickers.
; Use of this source code is governed by a BSD-style license that can
; be found in the MonkOS LICENSE file.
;=============================================================================

; The first-stage boot loader starts in 16-bit real mode.
bits 16

; All code addresses are offset from segment 0x7c00 (cs = 0x07c0), so we must
; use 0 as the origin.
org 0

; Produce a map file containing all symbols and sections.
[map all ../build/boot/boot.map]

; Include constants, structures, and macros.
%include "include/mem.inc"          ; Memory layout constants
%include "include/globals.inc"      ; Global variable definitions
%include "include/bios.inc"         ; BIOS structures
%include "include/iso9660.inc"      ; ISO9660 structures


;=============================================================================
; boot
;
; First-stage boot loader entry point
;
; The BIOS initialiates the boot process by running the code here in 16-bit
; real mode. The segment registers are all set to zero, and the instruction
; pointer (IP) is 0x7c00.
;
; Input registers:
;
;   AX      Boot signature (should be 0xaa55)
;   DL      Boot drive number
;
; Memory layout before this code starts running:
;
;   00000000 - 000003ff        1,024 bytes     Real mode IVT
;   00000400 - 000004ff          256 bytes     BIOS data area
;   00000500 - 00007bff       30,464 bytes     Free
;   00007c00 - 00007dff          512 bytes     First-stage boot loader (MBR)
;   00007e00 - 0009fbff      622,080 bytes     Free
;   0009fc00 - 0009ffff        1,024 bytes     Extended BIOS data area (EBDA)
;   000a0000 - 000bffff      131,072 bytes     BIOS video memory
;   000c0000 - 000fffff      196,608 bytes     ROM
;
;   [ See http://wiki.osdev.org/Memory_Map_(x86) ]
;
; Memory regions used or modified by this code:
;
;   00000500 - 000006ff          512 bytes     Global variables
;   00000800 - 00000fff        2,048 bytes     Cdrom sector read buffer
;   00001000 - 00007bff       27,648 bytes     Stack
;   00008000 - 0000ffff       32,768 bytes     Second-stage boot loader
;
;=============================================================================
boot:

    ; Do a far jump to update the code segment to 0x07c0.
    jmp     Mem.Loader1.Segment : .init

    ;-------------------------------------------------------------------------
    ; Initialize registers
    ;-------------------------------------------------------------------------
    .init:

        ; Disable interrupts while setting up stack pointer.
        cli

        ; All data segments (except es) are initialized to use the code
        ; segment.
        mov     ax,     cs
        mov     ds,     ax
        mov     fs,     ax
        mov     gs,     ax

        ; Set up a temporary stack.
        xor     ax,     ax
        mov     ss,     ax
        mov     sp,     Mem.Stack.Top

        ; The es segment is 0, which is useful for absolute addressing of the
        ; first 64KiB of memory.
        mov     es,     ax

        ; Re-enable interrupts.
        sti

        ; Store the BIOS boot drive number as a global variable so we can
        ; retrieve it later if necessary.
        mov     byte [es:Globals.DriveNumber], dl

    ;-------------------------------------------------------------------------
    ; Locate the ISO9660 sector containing the root directory
    ;-------------------------------------------------------------------------
    .findRootDirectory:

        ; Scan 2KiB sectors for the primary volume descriptor.
        mov     bx,     0x10                ; start with sector 0x10
        mov     cx,     1                   ; read 1 sector at a time
        mov     di,     Mem.Sector.Buffer   ; read into the sector buffer

        .readVolume:

            ; Read the sector containing the volume descriptor.
            call    ReadSectors
            jc      .error

            ; The volume's first byte contains its type.
            mov     al,     [es:Mem.Sector.Buffer]

            ; Type 1 is the primary volume descriptor.
            cmp     al,     0x01
            je      .found

            ; Type 0xff is the volume list terminator.
            cmp     al,     0xff
            je      .error

            ; Move on to next sector.
            inc     bx
            jmp     .readVolume

        .found:

            ; The primary volume descriptor contains a root directory entry,
            ; which specifies the sector containing the root directory.
            mov     bx,     [es:Mem.Sector.Buffer + \
                                ISO.PrimaryVolumeDescriptor.RootDirEntry + \
                                ISO.DirectoryEntry.LocationLBA]

            ; Store the root directory sector for use later.
            mov     [es:Globals.RootDirectorySector],   bx

    ;-------------------------------------------------------------------------
    ; Scan the root directory for the second-stage boot loader
    ;-------------------------------------------------------------------------
    .findLoader:

        .processSector:

            ; Load the current directory sector into the buffer.
            mov     cx,     1                   ; read 1 sector
            mov     di,     Mem.Sector.Buffer   ; read into sector buffer

            call    ReadSectors
            jc      .error

        .processDirEntry:

            ; Is the entry zero length? If so, we ran out of files in the
            ; directory.
            xor     ax,     ax
            mov     al,     [es:di + ISO.DirectoryEntry.RecordLength]
            cmp     al,     0
            je      .error

            ; Is the entry a file (flags & 2 == 0)?
            test    byte [es:di + ISO.DirectoryEntry.FileFlags],    0x02
            jnz     .nextDirEntry

            ; Is the file name the same length as "LOADER.SYS;1"?
            cmp     byte [es:di + ISO.DirectoryEntry.NameLength], \
                        Loader.Filename.Length
            jne     .nextDirEntry

            ; Is the file name "LOADER.SYS;1"?
            push    di
            mov     cx,     Loader.Filename.Length
            mov     si,     Loader.Filename
            add     di,     ISO.DirectoryEntry.Name
            cld
            rep     cmpsb
            pop     di
            je      .loaderFound

        .nextDirEntry:

            ; Advance to the next directory entry.
            add     di,     ax
            cmp     di,     Mem.Sector.Buffer + Mem.Sector.Buffer.Size
            jb      .processDirEntry

        .nextSector:

            ; Advance to the next directory sector.
            inc     bx
            jmp     .processSector

        .loaderFound:

            ; Display a status message.
            mov     si,     String.Loader.Found
            call    DisplayString

    ;-------------------------------------------------------------------------
    ; Read the second-stage loader from disk
    ;-------------------------------------------------------------------------
    .readLoader:

        ; Get the starting sector of the second-stage boot loader.
        mov     bx,     [es:di + ISO.DirectoryEntry.LocationLBA]

        .calcSize:

            ; If the upper word of the size dword is non-zero, then the loader
            ; is at least 64KiB, so it's too large.
            mov     cx,     [es:di + ISO.DirectoryEntry.Size + 2]
            cmp     cx,     0
            jne     .error

            ; Read the lower word for the size in bytes.
            mov     cx,     [es:di + ISO.DirectoryEntry.Size]

            ; Max size is 32KiB.
            cmp     cx,     0x8000
            ja      .error

            ; Divide loader size by 2KiB to get size in sectors.
            add     cx,     Mem.Sector.Buffer.Size - 1  ; Force rounding up.
            shr     cx,     11                          ; Divide by 2048.

        .load:

            ; Initialize es:di with target address for loader.
            mov     ax,     Mem.Loader2.Segment
            mov     es,     ax
            xor     di,     di      ; Assumes Mem.Loader2 % 16 == 0

            ; Read the second-stage boot loader into memory.
            call    ReadSectors
            jc      .error

    ;-------------------------------------------------------------------------
    ; Launch the second-stage boot loader
    ;-------------------------------------------------------------------------
    .launchLoader:

        ; Display a message indicating success.
        mov     si,     String.Loader.Starting
        call    DisplayString

        ; Do a far jump to the second-stage boot loader.
        jmp     0x0000 : Mem.Loader2

    ;-------------------------------------------------------------------------
    ; Error handling
    ;-------------------------------------------------------------------------
    .error:

        ; Display an error message
        mov     si,     String.Fail
        call    DisplayString

        .hang:

            ; Lock up the system.
            cli
            hlt
            jmp     .hang


;=============================================================================
; ReadSectors
;
; Read 1 or more 2048-byte sectors from the CDROM using int 13 function 42.
;
; Input registers:
;   BX      Starting sector LBA
;   CX      Number of sectors to read
;   DL      Drive number
;   ES:DI   Target buffer
;
; Return registers:
;   AH      Return code from int 13 (42h) BIOS call
;
; Flags:
;   CF      Set on error
;
; Killed registers:
;   AX, SI
;=============================================================================
ReadSectors:

    ; Fill the DAP buffer.
    mov     word [BIOS.DAP.Buffer + BIOS.DAP.ReadSectors],          cx
    mov     word [BIOS.DAP.Buffer + BIOS.DAP.TargetBufferOffset],   di
    mov     word [BIOS.DAP.Buffer + BIOS.DAP.TargetBufferSegment],  es
    mov     word [BIOS.DAP.Buffer + BIOS.DAP.FirstSector],          bx

    ; Load ds:si with the DAP buffer's address.
    lea     si,     [BIOS.DAP.Buffer]

    ; Call int13 BIOS function 42h (extended read sectors from drive).
    mov     ax,     0x4200
    int     0x13
    ret


;=============================================================================
; DisplayString
;
; Display a null-terminated string to the console using the BIOS int 10
; function 0E.
;
; Input registers:
;   SI      String offset
;
; Killed registers:
;   None
;=============================================================================
DisplayString:

    pusha

    mov     ah,     0x0e    ; int 10 AH=0x0e
    xor     bx,     bx

    cld

    .loop:

        ; Read next string character into al register.
        lodsb

        ; Break when a null terminator is reached.
        cmp     al,     0
        je      .done

        ; Call int 10 function 0eh (print character to teletype)
        int     0x10
        jmp     .loop

    .done:

        popa
        ret


;=============================================================================
; Global data
;=============================================================================

; The second-stage loader filename
Loader.Filename         db      "LOADER.SYS;1"
Loader.Filename.Length  equ     ($ - Loader.Filename)

; Display strings
String.Loader.Found     db      "[Monk] Loader found",          0x0d, 0x0a, 0
String.Loader.Starting  db      "[Monk] Loader starting",       0x0d, 0x0a, 0
String.Fail             db      "[Monk] ERROR: Failed to load", 0x0d, 0x0a, 0

; The DAP buffer used by ReadSectors
align 4
BIOS.DAP.Buffer:
    istruc BIOS.DAP
        at BIOS.DAP.Bytes,                  db   BIOS.DAP_size
        at BIOS.DAP.ReadSectors,            dw   0
        at BIOS.DAP.TargetBufferOffset,     dw   0
        at BIOS.DAP.TargetBufferSegment,    dw   0
        at BIOS.DAP.FirstSector,            dq   0
    iend


;=============================================================================
; Padding & boot signature
;=============================================================================

programEnd:

; Pad the boot record to 510 bytes
times   0x1fe - ($ - $$)    db  0

; Add the boot signature AA55 at the very end.
signature       dw      0xaa55
