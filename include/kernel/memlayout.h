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

// Segment selector values for segment registers.
#define SEGMENT_SELECTOR_KERNEL_DATA    0x08
#define SEGMENT_SELECTOR_KERNEL_CODE    0x10
#define SEGMENT_SELECTOR_USER_DATA      0x18
#define SEGMENT_SELECTOR_USER_CODE      0x20
#define SEGMENT_SELECTOR_TSS            0x28

// Memory layout
#define MEM_GLOBALS                   0x00000500
#define MEM_GDT                       0x00000700
#define MEM_TSS                       0x00000780
#define MEM_PAGETABLE                 0x00010000
#define MEM_PAGETABLE_PML4T           0x00010000
#define MEM_PAGETABLE_PDPT            0x00011000
#define MEM_PAGETABLE_PDT             0x00012000
#define MEM_PAGETABLE_PT0             0x00013000
#define MEM_PAGETABLE_PT1             0x00014000
#define MEM_PAGETABLE_PT2             0x00015000
#define MEM_PAGETABLE_PT3             0x00016000
#define MEM_PAGETABLE_PT4             0x00017000
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
