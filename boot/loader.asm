;=============================================================================
; @file loader.asm
;
; A second-stage boot loader suitable for an El Torito ISO 9660 cdrom image.
;
; This boot loader is executed after the first stage loader completes. Its
; responsibility is to load the kernel into memory, place the CPU into
; protected mode, then into 64-bit mode, and to start the kernel's execution.
;
; This loader allows for kernel images that are several megabytes in size.
; Making this possible was a bit tricky.  16-bit real mode is required to use
; the BIOS routines that load the kernel image from cdrom, but you can only
; use about 600KiB of memory while in real mode.  So this loader switches
; between real mode and 32-bit protected mode while transferring chunks of the
; kernel image from lower to upper memory.
;
; Copyright 2016 Brett Vickers.
; Use of this source code is governed by a BSD-style license that can
; be found in the MonkOS LICENSE file.
;=============================================================================

; The second-stage boot loader starts in 16-bit real mode.
bits 16

; The second-stage loader is executed from code segment 0, starting at address
; 0x8000.
org 0x8000

; Produce a map file containing all symbols and sections.
[map all ../build/boot/loader.map]

; Include constants, structures, and macros.
%include "include/mem.inc"          ; Memory layout constants
%include "include/globals.inc"      ; Global variable definitions
%include "include/bios.inc"         ; BIOS structures
%include "include/iso9660.inc"      ; ISO9660 structures
%include "include/gdt.inc"          ; Global descriptor table structures


;=============================================================================
; load
;
; This is the second stage loader's entry point.
;
; This code is launched by the first-stage loader and runs in 16-bit real mode
; starting at address 0:0x8000.
;
; Memory layout before this code starts running:
;
;   00000000 - 000003ff        1,024 bytes     Real mode IVT
;   00000400 - 000004ff          256 bytes     BIOS data area
;   00000500 - 000006ff          512 bytes     Global variables
;   00000700 - 00007bff       29,952 bytes     Free
;   00007c00 - 00007dff          512 bytes     First-stage boot loader (MBR)
;   00007e00 - 00097fff          512 bytes     Free
;   00008000 - 0000ffff       32,768 bytes     Second-stage boot loader
;   00010000 - 0009fbff      588,800 bytes     Free
;   0009fc00 - 0009ffff        1,024 bytes     Extended BIOS data area (EBDA)
;   000a0000 - 000bffff      131,072 bytes     BIOS video memory
;   000c0000 - 000fffff      196,608 bytes     ROM
;
;   [ See http://wiki.osdev.org/Memory_Map_(x86) ]
;
; Memory regions used or modified by this code:
;
;   00000700 - 000007ff          256 bytes     Global Descriptor Table (GDT)
;   00000800 - 00000fff        2,048 bytes     Cdrom sector read buffer
;   00001000 - 00007bff       27,648 bytes     Real mode stack
;   00010000 - 00017fff       32,768 bytes     Page tables
;   0006f000 - 0006ffff        4,096 bytes     32-bit protected mode stack
;   00070000 - 0007ffff       65,536 bytes     Kernel load buffer
;   00080000 - 0008ffff       65,536 bytes     Kernel stack
;   00100000 - (krnize)                        Kernel image
;
;=============================================================================
load:

    .init:

        ; Disable interrupts while updating segment registers and stack
        ; pointer.
        cli

        ; Initialize all segment registers to zero.
        xor     ax,     ax
        mov     ds,     ax
        mov     es,     ax
        mov     fs,     ax
        mov     gs,     ax
        mov     ss,     ax

        ; Initialize the stack pointer.
        mov     sp,     Mem.Stack.Top

        ; Clear all remaining general purpose registers.
        xor     bx,     bx
        xor     cx,     cx
        xor     dx,     dx
        xor     si,     si
        xor     di,     di
        xor     bp,     bp

    ;-------------------------------------------------------------------------
    ; Enable the A20 address line for upper memory access
    ;-------------------------------------------------------------------------
    .enableA20:

        ; Attempt to enable the A20 line if necessary.
        call    EnableA20
        jnc     .error.A20

        ; Display a status message.
        mov     si,     String.Status.A20Enabled
        call    DisplayStatusString

    ;-------------------------------------------------------------------------
    ; Make sure we have a 64-bit CPU
    ;-------------------------------------------------------------------------
    .detectLongMode:

        ; Detect if the cpuid instruction is available.
        call    HasCPUID
        jnc     .error.noCPUID

        ; Is the processor info function supported?
        mov     eax,    0x80000000  ; Get Highest Extended Function Supported
        cpuid
        cmp     eax,    0x80000001
        jb      .error.noLongMode

        ; Use processor info function to determine if long mode is supported.
        mov     eax,    0x80000001  ; Extended Processor Info and Feature Bits
        cpuid
        test    edx,    (1 << 29)   ; Long-mode bit
        jz      .error.noLongMode

        ; Clear 32-bit register values.
        xor     eax,    eax
        xor     edx,    edx

        ; Display a status message.
        mov     si,     String.Status.CPU64Detected
        call    DisplayStatusString

    ;-------------------------------------------------------------------------
    ; Enable SSE on the CPU
    ;-------------------------------------------------------------------------
    .enableSSE:

        ; Load CPU feature flags into ecx and edx.
        mov     eax,    1
        cpuid

        ; Check for SSE1 support.
        test    edx,    (1 << 25)
        jz      .error.noSSE

        ; Check for SSE2 support.
        test    edx,    (1 << 26)
        jz      .error.noSSE2

        ; Check for FXSAVE/FXRSTOR support.
        test    edx,    (1 << 24)
        jz      .error.noFXinst

        ; Enable hardware FPU with monitoring.
        mov     eax,    cr0
        and     eax,    ~(1 << 2)   ; turn off CR0.EM bit (x87 FPU is present)
        or      eax,    (1 << 1)    ; turn on CR0.MP bit (monitor FPU)
        mov     cr0,    eax

        ; Make sure FXSAVE/FXRSTOR instructions save the contents of the FPU
        ; MMX, XMM and MXCSR registers. Enable the use of SSE instructions.
        ; And indicate that the kernel is capable of handling SIMD floating-
        ; point exceptions.
        mov     eax,    cr4
        or      eax,    (1 << 9) | (1 << 10)    ; CR4.OFXSR, CR4.OSXMMEXCPT
        mov     cr4,    eax

        ; Display a status message.
        mov     si,     String.Status.SSEEnabled
        call    DisplayStatusString

    ;-------------------------------------------------------------------------
    ; Load the kernel image into upper memory
    ;-------------------------------------------------------------------------
    .loadKernel:

        ; Re-enable interrupts.
        sti

        ; Use a temporary GDT while loading the kernel, since we use 32-bit
        ; protected mode during the load.
        lgdt    [GDT32.Table.Pointer]

        ; Find the first sector and size of the kernel, with results in bx and
        ; eax.
        call    FindKernel
        jc      .error.kernelNotFound

        ; Display a status message.
        mov     si,     String.Status.KernelFound
        call    DisplayStatusString

        ; Start loading the kernel into memory.
        call    LoadKernel
        jc      .error.kernelLoadFailed

        ; Display a status message.
        mov     si,     String.Status.KernelLoaded
        call    DisplayStatusString

        ; Disable interrupts and leave them disabled until the kernel
        ; launches. The kernel is responsible for setting up an interrupt
        ; table before re-enabling interrupts.
        cli

    ;-------------------------------------------------------------------------
    ; Set up the 8259 programmable interrupt controller (PIC)
    ;-------------------------------------------------------------------------
    .setupPIC:

        ; Save IRQ masks.
        in      al,     0x21
        mov     cl,     al          ; cl = cached mask from data port 0x21
        in      al,     0xa1
        mov     ch,     al          ; ch = cached mask from data port 0xa1

        ; Start PIC initialization.
        mov     al,     0x11        ; 0x11 = INIT + ICW4_not_needed
        out     0x20,   al
        call    .wait
        out     0xa0,   al
        call    .wait

        ; Remap master PIC IRQs 0 thru 7 to interrupt numbers 0x20 thru 0x27.
        mov     al,     0x20        ; 0x20 = interrupt offset 32
        out     0x21,   al
        call    .wait

        ; Remap slave PIC IRQs 8 thru 15 to interrupt numbers 0x28 thru 0x2f.
        mov     al,     0x28        ; 0x28 = interrupt offset 40
        out     0xa1,   al
        call    .wait

        ; Tell master PIC there is a slave PIC at IRQ 4.
        mov     al,     4           ; 4 = IRQ 4
        out     0x21,   al
        call    .wait

        ; Tell slave PIC its cascade identity (IRQ2).
        mov     al,     2           ; 2 = IRQ 2
        out     0xa1,   al
        call    .wait

        ; Set 8086 mode.
        mov     al,     1           ; 1 = 8086/8088 mode.
        out     0x21,   al
        call    .wait
        out     0xa1,   al
        call    .wait

        ; Disable all IRQs. Kernel will re-enable the ones it wants to handle
        ; later.
        mov     al,     0xff
        out     0x21,   al
        out     0xa1,   al

        jmp     .setupGDT64

        .wait:

            xor     ax,     ax
            out     0x80,   al
            ret

    ;-------------------------------------------------------------------------
    ; Set up (but don't yet install) the 64-bit GDT
    ;-------------------------------------------------------------------------
    .setupGDT64:

        ; Set up a copy of the GDT to its memory layout location (0x0700).
        mov     si,     GDT64.Table
        mov     di,     Mem.GDT
        mov     cx,     GDT64.Table.Size
        shr     cx,     1           ; div table size by 2, copying words

        ; Do the copy.
        cld
        rep     movsw

    ;-------------------------------------------------------------------------
    ; Set up page tables
    ;-------------------------------------------------------------------------
    .setupPageTables:

        ; Create page tables that identity-map the first 10MiB of memory. This
        ; should be more than enough to hold the kernel.
        call    SetupPageTables

        ; Enable PAE paging.
        mov     eax,    cr4
        or      eax,    (1 << 5)
        mov     cr4,    eax

    ;-------------------------------------------------------------------------
    ; Enable long mode, protected mode, and paging
    ;-------------------------------------------------------------------------
    .enableLongMode:

        ; Enable long mode.
        mov     ecx,    0xc0000080
        rdmsr
        or      eax,    (1 << 8)
        wrmsr

        ; Enable paging and protected mode.
        mov     eax,    cr0
        or      eax,    (1 << 31) | (1 << 0)
        mov     cr0,    eax

        ; Load the 64-bit GDT.
        lgdt    [GDT64.Table.Pointer]

        ; Do a long jump using the new GDT, which forces the switch to 64-bit
        ; mode.
        jmp     GDT64.Selector.Code : .launch64

bits 64

    ;-------------------------------------------------------------------------
    ; Launch the 64-bit kernel
    ;-------------------------------------------------------------------------
    .launch64:

        ; Set up the data segment registers. Note that in 64-bit mode the CPU
        ; treats cs, ds, es, and ss as zero regardless of what we store there.
        ; (gs and fs are exceptions; they can be used as real segment
        ; registers.)
        xor     ax,     ax
        mov     ds,     ax
        mov     es,     ax
        mov     fs,     ax
        mov     gs,     ax
        mov     ss,     ax

        ; Set up a temporary stack, which the kernel should replace.
        mov     rsp,    Mem.Kernel.Stack

        ; Initialize all general purpose registers.
        xor     rax,    rax
        xor     rbx,    rbx
        xor     rcx,    rcx
        xor     rdx,    rdx
        xor     rdi,    rdi
        xor     rsi,    rsi
        xor     rbp,    rbp
        xor     r8,     r8
        xor     r9,     r9
        xor     r10,    r10
        xor     r11,    r11
        xor     r12,    r12
        xor     r13,    r13
        xor     r14,    r14
        xor     r15,    r15

        ; Do a jump to the kernel's entry point.
        jmp     Mem.Kernel.Code

bits 16

    ;-------------------------------------------------------------------------
    ; Error handlers
    ;-------------------------------------------------------------------------

    .error.A20:

        mov     si,     String.Error.A20
        jmp     .error

    .error.noCPUID:

        mov     si,     String.Error.NoCPUID
        jmp     .error

    .error.noLongMode:

        mov     si,     String.Error.NoLongMode
        jmp     .error

    .error.kernelNotFound:

        mov     si,     String.Error.KernelNotFound
        jmp     .error

    .error.kernelLoadFailed:

        mov     si,     String.Error.KernelLoadFailed
        jmp     .error

    .error.noSSE:

        mov     si,     String.Error.NoSSE
        jmp     .error

    .error.noSSE2:

        mov     si,     String.Error.NoSSE2
        jmp     .error

    .error.noFXinst:

        mov     si,     String.Error.NoFXinst
        jmp     .error

    .error:

        call    DisplayErrorString

    .hang:

        cli
        hlt
        jmp     .hang


;=============================================================================
; EnableA20
;
; Enable the A20 address line, so memory above 1MiB can be accessed.
;
; Returned flags:
;   CF      Set if enabled
;
; Killed registers:
;   None
;=============================================================================
EnableA20:

    ; Preserve ax register.
    push    ax

    ; Check if the A20 line is already enabled.
    call    TestA20
    jc      .done

    .attempt1:

        ; Attempt enabling with the BIOS.
        mov     ax,     0x2401
        int     0x15

        ; Check if A20 line is now enabled.
        call    TestA20
        jc      .done

    .attempt2:

        ; Attempt enabling with the keyboard controller.
        call    .attempt2.wait1

        ; Disable keyboard
        mov     al,     0xad
        out     0x64,   al
        call    .attempt2.wait1

        ; Read from input
        mov     al,     0xd0
        out     0x64,   al
        call    .attempt2.wait2

        ; Get keyboard data
        in      al,     0x60
        push    eax
        call    .attempt2.wait1

        ; Write to output
        mov     al,     0xd1
        out     0x64,   al
        call    .attempt2.wait1

        ; Send data
        pop     eax
        or      al,     2
        out     0x60,   al
        call    .attempt2.wait1

        ; Enable keyboard
        mov     al,     0xae
        out     0x64,   al
        call    .attempt2.wait1

        ; Check if the A20 line is now enabled.
        call    TestA20
        jc      .done

        jmp     .attempt3

        .attempt2.wait1:

            in      al,     0x64
            test    al,     2
            jnz     .attempt2.wait1
            ret

        .attempt2.wait2:

            in      al,     0x64
            test    al,     1
            jz      .attempt2.wait2
            ret

    .attempt3:

        ; Attempt enabling with the FAST A20 feature.
        in      al,     0x92
        or      al,     2
        out     0x92,   al
        xor     ax,     ax

        ; Check if A20 line is now enabled.
        call    TestA20

    .done:

        ; Restore register.
        pop     ax

        ret


;=============================================================================
; TestA20
;
; Check to see if the A20 address line is enabled.
;
; Return flags:
;   CF      Set if enabled
;
; Killed registers:
;   None
;=============================================================================
TestA20:

    ; Preserve registers.
    push    ds
    push    es
    pusha

    ; Initialize return result to "not enabled".
    clc

    ; Set es segment register to 0x0000.
    xor     ax,     ax
    mov     es,     ax

    ; Set ds segment register to 0xffff.
    not     ax
    mov     ds,     ax

    ; If the A20 line is disabled, then es:di and ds:si will point to the same
    ; physical memory location due to wrap-around at 1 MiB.
    ;
    ; es:di = 0000:0500 = 0x0000 * 16 + 0x0500 = 0x00500 = 0x0500
    ; ds:si = ffff:0510 = 0xffff * 16 + 0x0510 = 0x10500 = 0x0500
    mov     di,     0x0500
    mov     si,     0x0510

    ; Preserve the original values stored at es:di and ds:si.
    mov     ax,     [es:di]
    push    ax
    mov     ax,     [ds:si]
    push    ax

    ; Write different values to each logical address.
    mov     byte [es:di],   0x00
    mov     byte [ds:si],   0xff

    ; If a store to ds:si changes the value at es:di, then memory wrapped and
    ; A20 is not enabled.
    cmp     byte [es:di],   0xff

    ; Restore the original values stored at es:di and ds:si.
    pop     ax
    mov     [ds:si],    ax
    pop     ax
    mov     [es:di],    ax

    je      .done

    .enabled:

        ; Set the carry flag to indicate the A20 line is enabled.
        stc

    .done:

        ; Restore registers.
        popa
        pop     es
        pop     ds

        ret


;=============================================================================
; HasCPUID
;
; Detect if the cpu supports the CPUID instruction.
;
; Bit 21 of the EFLAGS register can be modified only if the CPUID instruction
; is supported.
;
; Return flags:
;   CF      Set if CPUID is supported
;
; Killed registers:
;   None
;=============================================================================
HasCPUID:

    ; Preserve registers.
    push    eax
    push    ecx

    ; Copy flags to eax and ecx.
    pushfd
    pop     eax
    mov     ecx,    eax

    ; Set flag 21 (the ID bit)
    xor     eax,    (1 << 21)
    push    eax
    popfd

    ; Copy flags back to eax. If CPUID is supported, bit 21 will still be set.
    pushfd
    pop     eax

    ; Restore the original flags from ecx.
    push    ecx
    popfd

    ; Initialize the return flag (carry) to unsupported.
    clc

    ; If eax and ecx are equal, then flag 21 didn't remain set, and CPUID is
    ; not supported.
    xor     eax,    ecx
    jz      .done       ; CPUID is not supported

    .supported:

        ; CPUID is supported.
        stc

    .done:

        ; Restore registers.
        pop     ecx
        pop     eax

        ret


;=============================================================================
; FindKernel
;
; Scan the root directory of the cdrom for a file called "MONK.SYS". If
; found, return its start sector and file size.
;
; Return registers:
;   EAX     Kernel file size in bytes
;   BX      Start sector
;
; Return flags:
;   CF      Set on error
;
; Killed registers:
;   None
;=============================================================================
FindKernel:

    ; Save registers that would otherwise be killed.
    push    cx
    push    dx
    push    si
    push    di

    ; Obtain the drive number and root directory sector from global variables.
    ; These were set by the first-stage boot loader.
    mov     dl,     [Globals.DriveNumber]
    mov     bx,     [Globals.RootDirectorySector]

    .processSector:

        ; Load the current directory sector into the buffer.
        mov     cx,     1                   ; cx = 1 sector
        mov     di,     Mem.Sector.Buffer   ; es:di = target buffer

        call    ReadSectors
        jc      .error

    .processDirEntry:

        ; Is the entry zero length? If so, we ran out of files in the
        ; directory.
        xor     ax,     ax
        mov     al,     [di + ISO.DirectoryEntry.RecordLength]
        cmp     al,     0
        je      .error

        ; Is the entry a file (flags & 2 == 0)?
        test    byte [di + ISO.DirectoryEntry.FileFlags],   0x02
        jnz     .nextDirEntry

        ; Is the file name the same length as "MONK.SYS;1"?
        cmp     byte [di + ISO.DirectoryEntry.NameLength], \
                    Kernel.Filename.Length
        jne     .nextDirEntry

        ; Is the file name "MONK.SYS;1"?
        push    di
        mov     cx,     Kernel.Filename.Length
        mov     si,     Kernel.Filename
        add     di,     ISO.DirectoryEntry.Name
        cld
        rep     cmpsb
        pop     di
        je      .kernelFound

    .nextDirEntry:

        ; Advance to the next directory entry.
        add     di,     ax
        cmp     di,     Mem.Sector.Buffer + Mem.Sector.Buffer.Size
        jb      .processDirEntry

    .nextSector:

        ; Advance to the next directory sector.
        inc     bx
        jmp     .processSector

    .kernelFound:

        ; Return starting sector in bx.
        mov     bx,     [di + ISO.DirectoryEntry.LocationLBA]

        ; Return file size in eax.
        mov     eax,    [di + ISO.DirectoryEntry.Size]

    .success:

        ; Clear carry to indicate success.
        clc

        jmp     .done

    .error:

        ; Set carry to indicate an error.
        stc

    .done:

        ; Restore registers.
        pop     di
        pop     si
        pop     dx
        pop     cx

        ret


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
; LoadKernel
;
; Load the kernel into upper memory.
;
; There are two problems we need to solve:
;
;   1. In real mode, we have access to the BIOS but can only access the
;      first megabyte of system memory.
;   2. In protected mode, we can access memory above the first megabyte but
;      don't have access to the BIOS.
;
; Since we need the BIOS to read the kernel from the CDROM and we need to load
; it into upper memory, we'll have to switch back and forth between real mode
; and protected mode to do it.
;
; This code repeats the following steps until the kernel is fully copied into
; upper memory:
;
;     1. Use BIOS to read the next 64KiB of the kernel file into a lower
;        memory buffer.
;     2. Switch to 32-bit protected mode.
;     3. Copy the 64KiB chunk from the lower memory buffer into the
;        appropriate upper memory location.
;     4. Switch back to real mode.
;
; Input registers:
;   EAX     Size of the kernel file
;   BX      Start sector of the kernel file
;
; Return flags:
;   CF      Set on error
;
; Killed registers:
;   None
;=============================================================================
LoadKernel:

    ; Preserve registers.
    pusha

    ; Preserve the real mode stack pointer.
    mov     [LoadKernel.StackPointer],  sp

    ; Retrieve the cdrom disk number.
    mov     dl,     [Globals.DriveNumber]

    ; Save the kernel size.
    mov     [Globals.KernelSize],       eax

    ; Convert kernel size from bytes to sectors (after rounding up).
    add     eax,    Mem.Sector.Buffer.Size - 1
    shr     eax,    11

    ; Store status in code memory, since it's hard to use the stack while
    ; switching between real and protected modes.
    mov     [LoadKernel.CurrentSector], bx
    add     ax,                         bx
    mov     [LoadKernel.LastSector],    ax

    .loadChunk:

        ; Set target buffer for the read.
        mov     cx,     Mem.Kernel.LoadBuffer.Segment
        mov     es,     cx
        xor     di,     di

        ; Set the number of sectors to read (buffersize / 2048).
        mov     cx,     Mem.Kernel.LoadBuffer.Size >> 11

        ; Calculate the number of remaining sectors.
        ; (ax = LastSector, bx = CurrentSector)
        sub     ax,     bx

        ; Are there fewer sectors to read than will fit in the buffer?
        cmp     cx,     ax
        jb      .proceed

        ; Don't read more sectors than are left.
        mov     cx,     ax

    .proceed:

        ; Store the number of sectors being loaded, so we can access it in
        ; protected mode when we do the copy to upper memory.
        mov     [LoadKernel.SectorsToCopy],     cx

        ; Read a chunk of the kernel into the buffer.
        call    ReadSectors
        jc      .errorReal

    .prepareProtected32Mode:

        ; Disable interrupts until we're out of protected mode and back into
        ; real mode, since we're not setting up a new interrupt table.
        cli

        ; Enable protected mode.
        mov     eax,    cr0
        or      eax,    (1 << 0)
        mov     cr0,    eax

        ; Do a far jump to switch to 32-bit protected mode.
        jmp     GDT32.Selector.Code32 : .switchToProtected32Mode

bits 32

    .switchToProtected32Mode:

        ; Initialize all data segment registers with the 32-bit protected mode
        ; data segment selector.
        mov     ax,     GDT32.Selector.Data32
        mov     ds,     ax
        mov     es,     ax
        mov     ss,     ax

        ; Create a temporary stack used only while in protected mode.
        ; (probably not necessary since interrupts are disabled)
        mov     esp,    Mem.Stack32.Temp.Top

    .copyChunk:

        ; Set up a copy from lower memory to upper memory using the number of
        ; sectors.
        xor     ecx,    ecx
        xor     esi,    esi
        xor     edi,    edi
        mov     bx,     [LoadKernel.SectorsToCopy]
        mov     cx,     bx
        shl     ecx,    11       ; multiply by sector size (2048)
        mov     esi,    Mem.Kernel.LoadBuffer
        mov     edi,    [LoadKernel.TargetPointer]

        ; Advance counters and pointers.
        add     [LoadKernel.TargetPointer],     ecx
        add     [LoadKernel.CurrentSector],     bx

        ; Copy the chunk.
        shr     ecx,    2       ; divide by 4 since we're copying dwords.
        cld
        rep     movsd

    .prepareProtected16Mode:

        ; Before we can switch back to real mode, we have to switch to
        ; 16-bit protected mode.
        jmp     GDT32.Selector.Code16 : .switchToProtected16Mode

bits 16

    .switchToProtected16Mode:

        ; Initialize all data segment registers with the 16-bit protected mode
        ; data segment selector.
        mov     ax,     GDT32.Selector.Data16
        mov     ds,     ax
        mov     es,     ax
        mov     ss,     ax

    .prepareRealMode:

        ; Disable protected mode.
        mov     eax,    cr0
        and     eax,    ~(1 << 0)
        mov     cr0,    eax

        ; Do a far jump to switch back to real mode.
        jmp     0x0000 : .switchToRealMode

    .switchToRealMode:

        ; Restore real mode data segment registers.
        xor     ax,     ax
        mov     ds,     ax
        mov     es,     ax
        mov     ss,     ax

        ; Restore the real mode stack pointer.
        xor     esp,    esp
        mov     sp,     [LoadKernel.StackPointer]

        ; Enable interrupts again.
        sti

    .checkCompletion:

        ; Check if the copy is complete.
        mov     ax,     [LoadKernel.LastSector]
        mov     bx,     [LoadKernel.CurrentSector]
        cmp     ax,     bx
        je      .success

        ; Proceed to the next chunk.
        jmp     .loadChunk

    .errorReal:

        ; Set carry flag on error.
        stc
        jmp     .done

    .success:

        ; Clear carry on success.
        clc

    .done:

        ; Clear upper word of 32-bit registers we used.
        xor     eax,    eax
        xor     ecx,    ecx
        xor     esi,    esi
        xor     edi,    edi

        ; Restore registers.
        popa

        ret

;-----------------------------------------------------------------------------
; LoadKernel state variables
;-----------------------------------------------------------------------------
align 4
LoadKernel.TargetPointer        dd      Mem.Kernel.Image
LoadKernel.CurrentSector        dw      0
LoadKernel.LastSector           dw      0
LoadKernel.SectorsToCopy        dw      0
LoadKernel.StackPointer         dw      0


;=============================================================================
; SetupPageTables
;
; Set up a page table for 64-bit long mode.
;
; This procedure creates an identity-mapped page table for the first 10MiB of
; physical memory.
;
; Killed registers:
;   None
;=============================================================================
SetupPageTables:

    ; Constants for page table bits
    .Present     equ     1 << 0
    .ReadWrite   equ     1 << 1
    .PageBits    equ     .Present | .ReadWrite

    ; Preserve registers
    pusha
    push    es

    .clearMemory:

        ; Clear all memory used to hold the page tables.
        mov     ax,     Mem.PageTable >> 4
        mov     es,     ax

        xor     eax,    eax
        xor     edi,    edi
        mov     ecx,    (Mem.PageTable.End - Mem.PageTable) >> 2

        cld
        rep     stosd

    .makeTables:

        ; PML4T entry 0 points to the PDPT.
        mov     edi,                    Mem.PageTable.PML4T & 0xffff
        mov     dword [es:edi],         Mem.PageTable.PDPT | .PageBits

        ; PDPT entry 0 points to the PDT.
        mov     edi,                    Mem.PageTable.PDPT & 0xffff
        mov     dword [es:edi],         Mem.PageTable.PDT | .PageBits

        ; PDT entries 0 through 5 point to page tables 0 through 5.
        mov     edi,                    Mem.PageTable.PDT & 0xffff
        mov     dword [es:edi + 0x00],  Mem.PageTable.PT0 | .PageBits
        mov     dword [es:edi + 0x08],  Mem.PageTable.PT1 | .PageBits
        mov     dword [es:edi + 0x10],  Mem.PageTable.PT2 | .PageBits
        mov     dword [es:edi + 0x18],  Mem.PageTable.PT3 | .PageBits
        mov     dword [es:edi + 0x20],  Mem.PageTable.PT4 | .PageBits

        ; Prepare to create page table entries.
        mov     edi,    Mem.PageTable.PT0 & 0xffff
        mov     eax,    .PageBits
        mov     ecx,    512 * 5     ; 5 contiguous page tables

    .makePage:

        ; Loop through each page table entry, incrementing the physical
        ; address by one page each time.
        mov     dword [es:edi],         eax     ; store addr + .PageBits
        add     eax,                    0x1000  ; next physical address
        add     edi,                    8       ; next page table entry
        loop    .makePage

    .initPageRegister:

        ; CR3 is the page directory base register.
        mov     edi,    Mem.PageTable
        mov     cr3,    edi

    .done:

        ; Clear the upper bits of 32-bit registers we used.
        xor     eax,    eax
        xor     ecx,    ecx
        xor     edi,    edi

        ; Restore registers.
        pop     es
        popa

        ret


;=============================================================================
; DisplayStatusString
;
; Add an OS prefix and a CRLF suffix to a null-terminated string, then display
; it to the console using the BIOS.
;
; Input registers:
;   SI      String offset
;
; Killed registers:
;   None
;=============================================================================
DisplayStatusString:

    push    si

    mov     si,     String.OS.Prefix
    call    DisplayString

    pop     si

    call    DisplayString

    mov     si,     String.CRLF
    call    DisplayString

    ret


;=============================================================================
; DisplayErrorString
;
; Add an OS+error prefix and a CRLF suffix to a null-terminated string, then
; display it to the console using the BIOS.
;
; Input registers:
;   SI      String offset
;
; Killed registers:
;   None
;=============================================================================
DisplayErrorString:

    push    si

    mov     si,     String.OS.Prefix
    call    DisplayString

    mov     si,     String.Error.Prefix
    call    DisplayString

    pop     si

    call    DisplayString

    mov     si,     String.CRLF
    call    DisplayString

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

    push    ax
    push    bx

    mov     ah,     0x0e    ; int 10 AH=0x0e
    xor     bx,     bx      ; page + color

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

        pop     bx
        pop     ax

        ret


;=============================================================================
; Global data
;=============================================================================

;-----------------------------------------------------------------------------
; Status and error strings
;-----------------------------------------------------------------------------

String.CRLF                   db 0x0d, 0x0a,                0

String.OS.Prefix              db "[Monk] ",                 0

String.Status.A20Enabled      db "A20 line enabled",        0
String.Status.CPU64Detected   db "64-bit CPU detected",     0
String.Status.SSEEnabled      db "SSE enabled",             0
String.Status.KernelFound     db "Kernel found",            0
String.Status.KernelLoaded    db "Kernel loaded",           0

String.Error.Prefix           db "ERROR: ",                 0
String.Error.A20              db "A20 line not enabled",    0
String.Error.NoCPUID          db "CPUID not supported",     0
String.Error.NoLongMode       db "CPU is not 64-bit",       0
String.Error.NoSSE            db "No SSE support",          0
String.Error.NoSSE2           db "No SSE2 support",         0
String.Error.NoFXinst         db "No FXSAVE/FXRSTOR",       0
String.Error.KernelNotFound   db "Kernel not found",        0
String.Error.KernelLoadFailed db "Kernel load failed",      0

;-----------------------------------------------------------------------------
; Filename strings
;-----------------------------------------------------------------------------

Kernel.Filename         db      "MONK.SYS;1"
Kernel.Filename.Length  equ     ($ - Kernel.Filename)

;-----------------------------------------------------------------------------
; DAP buffer used by ReadSectors
;-----------------------------------------------------------------------------
align 4
BIOS.DAP.Buffer:
    istruc BIOS.DAP
        at BIOS.DAP.Bytes,                  db   BIOS.DAP_size
        at BIOS.DAP.ReadSectors,            dw   0
        at BIOS.DAP.TargetBufferOffset,     dw   0
        at BIOS.DAP.TargetBufferSegment,    dw   0
        at BIOS.DAP.FirstSector,            dq   0
    iend

;-----------------------------------------------------------------------------
; Global Descriptor Table used (temporarily) in 32-bit protected mode
;-----------------------------------------------------------------------------
align 4
GDT32.Table:

    ; Null descriptor
    istruc GDT.Descriptor
        at GDT.Descriptor.LimitLow,            dw      0x0000
        at GDT.Descriptor.BaseLow,             dw      0x0000
        at GDT.Descriptor.BaseMiddle,          db      0x00
        at GDT.Descriptor.Access,              db      0x00
        at GDT.Descriptor.LimitHighFlags,      db      0x00
        at GDT.Descriptor.BaseHigh,            db      0x00
    iend

    ; 32-bit protected mode - code segment descriptor (selector = 0x08)
    ; (Base=0, Limit=4GiB-1, RW=1, DC=0, EX=1, PR=1, Priv=0, SZ=1, GR=1)
    istruc GDT.Descriptor
        at GDT.Descriptor.LimitLow,            dw      0xffff
        at GDT.Descriptor.BaseLow,             dw      0x0000
        at GDT.Descriptor.BaseMiddle,          db      0x00
        at GDT.Descriptor.Access,              db      10011010b
        at GDT.Descriptor.LimitHighFlags,      db      11001111b
        at GDT.Descriptor.BaseHigh,            db      0x00
    iend

    ; 32-bit protected mode - data segment descriptor (selector = 0x10)
    ; (Base=0, Limit=4GiB-1, RW=1, DC=0, EX=0, PR=1, Priv=0, SZ=1, GR=1)
    istruc GDT.Descriptor
        at GDT.Descriptor.LimitLow,            dw      0xffff
        at GDT.Descriptor.BaseLow,             dw      0x0000
        at GDT.Descriptor.BaseMiddle,          db      0x00
        at GDT.Descriptor.Access,              db      10010010b
        at GDT.Descriptor.LimitHighFlags,      db      11001111b
        at GDT.Descriptor.BaseHigh,            db      0x00
    iend

    ; 16-bit protected mode - code segment descriptor (selector = 0x18)
    ; (Base=0, Limit=1MiB-1, RW=1, DC=0, EX=1, PR=1, Priv=0, SZ=0, GR=0)
    istruc GDT.Descriptor
        at GDT.Descriptor.LimitLow,            dw      0xffff
        at GDT.Descriptor.BaseLow,             dw      0x0000
        at GDT.Descriptor.BaseMiddle,          db      0x00
        at GDT.Descriptor.Access,              db      10011010b
        at GDT.Descriptor.LimitHighFlags,      db      00000001b
        at GDT.Descriptor.BaseHigh,            db      0x00
    iend

    ; 16-bit protected mode - data segment descriptor (selector = 0x20)
    ; (Base=0, Limit=1MiB-1, RW=1, DC=0, EX=0, PR=1, Priv=0, SZ=0, GR=0)
    istruc GDT.Descriptor
        at GDT.Descriptor.LimitLow,            dw      0xffff
        at GDT.Descriptor.BaseLow,             dw      0x0000
        at GDT.Descriptor.BaseMiddle,          db      0x00
        at GDT.Descriptor.Access,              db      10010010b
        at GDT.Descriptor.LimitHighFlags,      db      00000001b
        at GDT.Descriptor.BaseHigh,            db      0x00
    iend

GDT32.Table.Size    equ     ($ - GDT32.Table)

GDT32.Table.Pointer:
    dw  GDT32.Table.Size - 1    ; Limit = offset of last byte in table
    dd  GDT32.Table


;-----------------------------------------------------------------------------
; Global Descriptor Table used in 64-bit long mode
;-----------------------------------------------------------------------------
align 8
GDT64.Table:

    ; Null descriptor
    istruc GDT.Descriptor
        at GDT.Descriptor.LimitLow,            dw      0x0000
        at GDT.Descriptor.BaseLow,             dw      0x0000
        at GDT.Descriptor.BaseMiddle,          db      0x00
        at GDT.Descriptor.Access,              db      0x00
        at GDT.Descriptor.LimitHighFlags,      db      0x00
        at GDT.Descriptor.BaseHigh,            db      0x00
    iend

    ; 64-bit long mode code segment descriptor (selector = 0x08)
    istruc GDT.Descriptor
        at GDT.Descriptor.LimitLow,            dw      0x0000
        at GDT.Descriptor.BaseLow,             dw      0x0000
        at GDT.Descriptor.BaseMiddle,          db      0x00
        at GDT.Descriptor.Access,              db      10011010b
        at GDT.Descriptor.LimitHighFlags,      db      00100000b
        at GDT.Descriptor.BaseHigh,            db      0x00
    iend

    ; 64-bit long mode data segment descriptor (selector = 0x10)
    istruc GDT.Descriptor
        at GDT.Descriptor.LimitLow,            dw      0x0000
        at GDT.Descriptor.BaseLow,             dw      0x0000
        at GDT.Descriptor.BaseMiddle,          db      0x00
        at GDT.Descriptor.Access,              db      10010010b
        at GDT.Descriptor.LimitHighFlags,      db      00000000b
        at GDT.Descriptor.BaseHigh,            db      0x00
    iend

GDT64.Table.Size    equ     ($ - GDT64.Table)

; GDT64 table pointer
; This pointer references Mem.GDT, not GDT64.Table, because that's where the
; long mode GDT will reside after we copy it.
GDT64.Table.Pointer:
    dw  GDT64.Table.Size - 1    ; Limit = offset of last byte in table
    dq  Mem.GDT                 ; Address of table copy


;=============================================================================
; Padding
;=============================================================================

programEnd:

; Pad the boot record to 32 KiB
times   0x8000 - ($ - $$)    db  0
