//============================================================================
/// @file       heap.h
/// @brief      A simple heap memory manager
//
//  Copyright 2016 Brett Vickers.
//  Use of this source code is governed by a BSD-style license
//  that can be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <core.h>
#include <kernel/mem/paging.h>

typedef struct heap heap_t;

//----------------------------------------------------------------------------
//  @function   heap_create
/// @brief      Create a new heap from which to allocate virtual memory.
/// @param[in]  pt          The page table from which virtual memory is to be
///                         allocated.
/// @param[in]  vaddr       The virtual address of the first byte to use for
///                         the heap.
/// @param[in]  maxpages    The maximum number of pages that the heap will
///                         grow to fill.
/// @returns    A pointer to a the created heap structure.
//----------------------------------------------------------------------------
heap_t *
heap_create(pagetable_t *pt, void *vaddr, uint64_t maxpages);

//----------------------------------------------------------------------------
//  @function   heap_destroy
/// @brief      Destroy a heap, returning its memory to page table.
/// @param[in]  heap        The heap to be destroyed.
//----------------------------------------------------------------------------
void
heap_destroy(heap_t *heap);

//----------------------------------------------------------------------------
//  @function   heap_alloc
/// @brief      Allocate memory from a heap.
/// @param[in]  heap    The heap from which to allocate the memory.
/// @param[in]  size    The size, in bytes, of the allocation.
/// @returns    A pointer to a the allocated memory.
//----------------------------------------------------------------------------
void *
heap_alloc(heap_t *heap, uint64_t size);

//----------------------------------------------------------------------------
//  @function   heap_free
/// @brief      Free memory previously allocated with heap_alloc.
/// @param[in]  heap    The heap from which the memory was allocated.
/// @param[in]  ptr     A pointer to the memory that was allocated.
//----------------------------------------------------------------------------
void
heap_free(heap_t *heap, void *ptr);
