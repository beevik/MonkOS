//============================================================================
/// @file   memlayout.h
/// @brief  Kernel memory layout
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <core.h>

//----------------------------------------------------------------------------
// Physical memory layout
//----------------------------------------------------------------------------
// 00000000 - 000000ff          256 bytes   Free
// 00000100 - 000001ff          256 bytes   Global Descriptor Table (GDT)
// 00000200 - 000002ff          256 bytes   Task State Segment (TSS)
// 00000300 - 000004ff          512 bytes   Free
// 00000500 - 000006ff          512 bytes   Global variables
// 00000700 - 0000ffff       63,744 bytes   Free
// 00010000 - 00017fff       32,768 bytes   Page tables for first 10MiB
// 00018000 - 0001bfff       16,384 bytes   Memory layout (from BIOS)
// 0001c000 - 00089fff      450,560 bytes   Free
// 0008a000 - 0008bfff        8,192 bytes   Exception stack (NMI)
// 0008c000 - 0008cfff        8,192 bytes   Exception stack (DF)
// 0008d000 - 0008ffff        8,192 bytes   Exception stack (MC)
// 00090000 - 0009dbff       57,344 bytes   Reserved
// 0009e000 - 0009ffff        8,192 bytes   Extended BIOS data area (EBDA)
// 000a0000 - 000bffff      131,072 bytes   Video memory
// 000c0000 - 000fffff      262,144 bytes   ROM
// 00100000 - 001fffff    1,048,576 bytes   Kernel stack
// 00200000 - 002fffff    1,048,576 bytes   Kernel interrupt stack
// 00300000 - (krnize)                      Kernel image
//----------------------------------------------------------------------------

// Segment selector values for segment registers.
#define SEGMENT_SELECTOR_KERNEL_DATA    0x08
#define SEGMENT_SELECTOR_KERNEL_CODE    0x10
#define SEGMENT_SELECTOR_USER_DATA      0x18
#define SEGMENT_SELECTOR_USER_CODE      0x20
#define SEGMENT_SELECTOR_TSS            0x28

// Physical memory layout constants
#define MEM_GDT                       0x00000100
#define MEM_TSS                       0x00000200
#define MEM_GLOBALS                   0x00000500
#define MEM_IDT                       0x00001000
#define MEM_ISR_TABLE                 0x00002000
#define MEM_ISR_THUNKS                0x00002800
#define MEM_PAGETABLE                 0x00010000
#define MEM_PAGETABLE_PML4T           0x00010000
#define MEM_PAGETABLE_PDPT            0x00011000
#define MEM_PAGETABLE_PDT             0x00012000
#define MEM_PAGETABLE_PT0             0x00013000
#define MEM_PAGETABLE_PT1             0x00014000
#define MEM_PAGETABLE_PT2             0x00015000
#define MEM_PAGETABLE_PT3             0x00016000
#define MEM_PAGETABLE_PT4             0x00017000
#define MEM_LAYOUT_BIOS               0x00018000
#define MEM_STACK_NMI_BOTTOM          0x0008a000
#define MEM_STACK_NMI_TOP             0x0008c000
#define MEM_STACK_DF_BOTTOM           0x0008c000
#define MEM_STACK_DF_TOP              0x0008e000
#define MEM_STACK_MC_BOTTOM           0x0008e000
#define MEM_STACK_MC_TOP              0x00090000
#define MEM_STACK_INTERRUPT_BOTTOM    0x00100000
#define MEM_STACK_INTERRUPT_TOP       0x00200000
#define MEM_STACK_KERNEL_BOTTOM       0x00200000
#define MEM_STACK_KERNEL_TOP          0x00300000
#define MEM_KERNEL_IMAGE              0x00300000
#define MEM_KERNEL_ENTRYPOINT         0x00301000
