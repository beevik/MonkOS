//============================================================================
/// @file       heap.c
/// @brief      Heap memory manager
//
//  Copyright 2016 Brett Vickers.
//  Use of this source code is governed by a BSD-style license
//  that can be found in the MonkOS LICENSE file.
//============================================================================

#include <core.h>
#include <kernel/mem/paging.h>

struct heap
{
    uint32_t              size;         // size of this heap in bytes
    uint32_t              reserved;
    struct heap          *root_heap;    // first heap in the heap chain
    struct heap          *next_heap;    // next heap in the heap chain
    struct fblock_header *first_fblock; // first free block in the heap
};

struct block_header
{
    uint32_t size;          // size of this (free or allocated) block
    uint32_t heap_offset;   // offset from the start of the block's heap
};

struct block_footer
{
    uint32_t size;       // size of the preceding block
};

struct fblock_header
{
    struct block_header   block;
    struct fblock_header *next_fblock;  // next fblock in the heap
    struct fblock_header *prev_fblock;  // prev fblock in the heap
};

struct heap *
heap_create(pagetable_t *pt, void *vaddr, uint32_t chunk_size)
{
    chunk_size = ((chunk_size + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
    int count = (int)(chunk_size / PAGE_SIZE);

    struct heap *heap = (struct heap *)page_alloc(pt, vaddr, count);
    heap->size         = chunk_size;
    heap->reserved     = 0;
    heap->root_heap    = heap;
    heap->next_heap    = NULL;
    heap->first_fblock = (struct fblock_header *)(heap + 1);

    uint32_t block_size = chunk_size - sizeof(struct heap) -
                          sizeof(struct block_header) -
                          sizeof(struct block_footer);

    struct fblock_header *fblock = heap->first_fblock;
    fblock->block.size        = block_size;
    fblock->block.heap_offset = sizeof(struct heap);
    fblock->next_fblock       = NULL;
    fblock->prev_fblock       = NULL;

    struct block_footer *footer =
        (struct block_footer *)((uint8_t *)(fblock + 1) + block_size -
                                sizeof(struct block_footer));
    footer->size = block_size;

    return heap;
}
