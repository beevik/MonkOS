//============================================================================
/// @file       kmem.h
/// @brief      Kernel physical (and virtual) memory map.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

// Kernel physical (and virtual) memory layout
#define KMEM_IDT                     0x00001000
#define KMEM_ISR_TABLE               0x00002000
#define KMEM_ISR_THUNKS              0x00002800
#define KMEM_GDT                     0x00003000
#define KMEM_TSS                     0x00003100
#define KMEM_GLOBALS                 0x00003200
#define KMEM_BOOT_PAGETABLE          0x00010000
#define KMEM_BOOT_PAGETABLE_LOADED   0x00014000
#define KMEM_BOOT_PAGETABLE_END      0x00020000
#define KMEM_KERNEL_PAGETABLE        0x00020000
#define KMEM_KERNEL_PAGETABLE_END    0x00070000
#define KMEM_TABLE_BIOS              0x00070000
#define KMEM_STACK_NMI_BOTTOM        0x0008a000
#define KMEM_STACK_NMI_TOP           0x0008c000
#define KMEM_STACK_DF_BOTTOM         0x0008c000
#define KMEM_STACK_DF_TOP            0x0008e000
#define KMEM_STACK_MC_BOTTOM         0x0008e000
#define KMEM_STACK_MC_TOP            0x00090000
#define KMEM_EXTENDED_BIOS           0x0009f800
#define KMEM_VIDEO                   0x000a0000
#define KMEM_SYSTEM_ROM              0x000c0000
#define KMEM_STACK_INTERRUPT_BOTTOM  0x00100000
#define KMEM_STACK_INTERRUPT_TOP     0x00200000
#define KMEM_STACK_KERNEL_BOTTOM     0x00200000
#define KMEM_STACK_KERNEL_TOP        0x00300000
#define KMEM_KERNEL_IMAGE            0x00300000
#define KMEM_KERNEL_ENTRYPOINT       0x00301000
#define KMEM_KERNEL_IMAGE_END        0x00a00000

#define KMEM_EXTENDED_BIOS_SIZE      0x00000800
#define KMEM_VIDEO_SIZE              0x00020000
#define KMEM_SYSTEM_ROM_SIZE         0x00040000
#define KMEM_KERNEL_PAGETABLE_SIZE   0x00050000

//----------------------------------------------------------------------------
//  @function   kmem_init
/// @brief      Using the contents of the physical memory map, identity-map
///             all physical memory into the kernel's page table.
/// @returns    Physical address of the kernel page table.
//----------------------------------------------------------------------------
uint64_t
kmem_init();

//----------------------------------------------------------------------------
//  @function   kmem_pagetable_addr
/// @brief      Return the physical address of the kernel's page table.
/// @returns    Physical address of the kernel page table.
//----------------------------------------------------------------------------
uint64_t
kmem_pagetable_addr();
