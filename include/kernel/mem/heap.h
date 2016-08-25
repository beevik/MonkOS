//============================================================================
/// @file       heap.h
/// @brief      Heap memory manager
//
//  Copyright 2016 Brett Vickers.
//  Use of this source code is governed by a BSD-style license
//  that can be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <core.h>
#include <kernel/mem/paging.h>

//----------------------------------------------------------------------------
//  @function   heap_create
/// @brief      Create a new heap from which to allocate virtual memory.
/// @param[in]  pt      The page table from which virtual memory is to be
///                     allocated.
/// @param[in]  vaddr   The virtual address of the first byte to use for the
///                     heap.
/// @param[in]  chunk_size  The size of each chunk within the heap.
/// @returns    A pointer to a the created heap structure.
//----------------------------------------------------------------------------
struct heap *
heap_create(pagetable_t *pt, void *vaddr, uint32_t chunk_size);
