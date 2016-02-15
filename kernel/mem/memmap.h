//============================================================================
/// @file       memmap.h
/// @brief      Kernel's physical memory map.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

// Physical memory layout supplied by the boot loader
#define MEM_GDT                       0x00000100
#define MEM_TSS                       0x00000200
#define MEM_GLOBALS                   0x00000500
#define MEM_IDT                       0x00001000
#define MEM_ISR_TABLE                 0x00002000
#define MEM_ISR_THUNKS                0x00002800
#define MEM_PAGETABLE                 0x00010000
#define MEM_PAGETABLE_PML4T           0x00010000
#define MEM_PAGETABLE_PDPT0           0x00011000
#define MEM_PAGETABLE_PDT0            0x00012000
#define MEM_PAGETABLE_PT0             0x00013000
#define MEM_TABLE_BIOS                0x00070000
#define MEM_STACK_NMI_BOTTOM          0x0008a000
#define MEM_STACK_NMI_TOP             0x0008c000
#define MEM_STACK_DF_BOTTOM           0x0008c000
#define MEM_STACK_DF_TOP              0x0008e000
#define MEM_STACK_MC_BOTTOM           0x0008e000
#define MEM_STACK_MC_TOP              0x00090000
#define MEM_VIDEO                     0x000a0000
#define MEM_SYSTEM_ROM                0x000c0000
#define MEM_STACK_INTERRUPT_BOTTOM    0x00100000
#define MEM_STACK_INTERRUPT_TOP       0x00200000
#define MEM_STACK_KERNEL_BOTTOM       0x00200000
#define MEM_STACK_KERNEL_TOP          0x00300000
#define MEM_KERNEL_IMAGE              0x00300000
#define MEM_KERNEL_ENTRYPOINT         0x00301000
#define MEM_KERNEL_IMAGE_END          0x00a00000
