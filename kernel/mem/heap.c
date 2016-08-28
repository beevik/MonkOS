//============================================================================
/// @file       heap.c
/// @brief      Heap memory manager
//
//  Copyright 2016 Brett Vickers.
//  Use of this source code is governed by a BSD-style license
//  that can be found in the MonkOS LICENSE file.
//============================================================================

#include <core.h>
#include <kernel/mem/heap.h>
#include <kernel/mem/paging.h>

// Constants
#define INITIAL_PAGES   16

// block_header flags
#define FLAG_ALLOCATED  (1 << 0)

// ROUND16: return the first value x greater than or equal to n that satisfies
//          (x mod 16) = r
#define ROUND16(n, r)  (((((n) - (r) + 31) >> 4) << 4) + (r) - 16)

struct heap
{
    void                 *vaddr;        // address of heap start
    uint64_t              pages;        // pages currently alloced to the heap
    uint64_t              maxpages;     // max pages used by the heap
    struct fblock_header *first_fblock; // first free block in the heap
};

typedef struct block_header
{
    uint64_t size;          // size of block in bytes, not incl header/footer
    uint64_t flags;
} block_header_t;

typedef struct block_footer
{
    uint64_t size;          // size of the preceding block
} block_footer_t;

typedef struct fblock_header
{
    struct block_header   block;
    struct fblock_header *next_fblock;  // next fh in the heap
    struct fblock_header *prev_fblock;  // prev fh in the heap
} fblock_header_t;

heap_t *
heap_create(pagetable_t *pt, void *vaddr, uint64_t maxpages)
{
    heap_t *heap = (heap_t *)page_alloc(pt, vaddr, INITIAL_PAGES);
    heap->vaddr        = vaddr;
    heap->pages        = INITIAL_PAGES;
    heap->maxpages     = max(INITIAL_PAGES, maxpages);
    heap->first_fblock = (fblock_header_t *)(heap + 1);

    uint64_t block_size = heap->pages * PAGE_SIZE - sizeof(heap_t) -
                          sizeof(block_header_t) -
                          sizeof(block_footer_t);

    fblock_header_t *fh = heap->first_fblock;
    fh->block.size  = block_size;
    fh->block.flags = 0;
    fh->next_fblock = NULL;
    fh->prev_fblock = NULL;

    block_footer_t *footer =
        (block_footer_t *)((uint8_t *)fh + block_size +
                           sizeof(block_header_t));
    footer->size = block_size;

    return heap;
}

// Find a free block large enough to hold 'size' bytes. If such a block is not
// found, grow the heap.
static fblock_header_t *
find_fblock(heap_t *heap, uint64_t size)
{
    fblock_header_t *fb = heap->first_fblock;
    while (fb) {
        if (fb->block.size >= size)
            return fb;
        fb = fb->next_fblock;
    }

    // TODO: Grow the heap
    return NULL;
}

void *
heap_alloc(heap_t *heap, uint64_t size)
{
    // Round the size up to the nearest (mod 16) == 8 value. This way all
    // returned pointers will remain aligned on 16-byte boundaries.
    size = ROUND16(size, 8);

    // Find a free block big enough to hold the allocation
    fblock_header_t *fb = find_fblock(heap, size);
    if (fb == NULL)
        return NULL;

    // Copy the original free block's size and pointers, since we'll be
    // modifying their current storage.
    uint64_t         fsize = fb->block.size;
    fblock_header_t *next  = fb->next_fblock;
    fblock_header_t *prev  = fb->prev_fblock;

    // If the free block would be filled (or nearly filled) by the
    // allocation, replace it with an allocated block.
    block_header_t *ah;
    if (fsize - size <= 8) {

        // Mark the block header as allocated, leaving the size unchanged.
        ah        = &fb->block;
        ah->flags = FLAG_ALLOCATED;

        // Ignore the allocated block footer, since the size remains
        // unchanged.

        // Fix up the free list.
        if (prev)
            prev->next_fblock = next;
        else
            heap->first_fblock = next;
        if (next)
            next->prev_fblock = prev;
    }

    // Otherwise, split the free block into an allocated block and a smaller
    // free block.
    else {

        // Initialize the allocated block header.
        ah        = &fb->block;
        ah->size  = size;
        ah->flags = FLAG_ALLOCATED;

        // Initialize the allocated block footer.
        block_footer_t *af =
            (block_footer_t *)((uint8_t *)ah + size + sizeof(block_header_t));
        af->size = size;

        // Initialize the new free block header, which follows the allocated
        // block.
        fblock_header_t *fh = (fblock_header_t *)(af + 1);
        fh->block.size = fsize - size - sizeof(block_header_t) -
                         sizeof(block_footer_t);
        fh->block.flags = 0;

        // Initialize the new free block footer.
        block_footer_t *ff =
            (block_footer_t *)((uint8_t *)fh + fh->block.size +
                               sizeof(block_header_t));
        ff->size = fh->block.size;

        // Fix up the free list.
        fh->prev_fblock = prev;
        fh->next_fblock = next;
        if (prev)
            prev->next_fblock = fh;
        else
            heap->first_fblock = fh;
        if (next)
            next->prev_fblock = fh;
    }

    // Return a pointer just beyond the allocated block header.
    return (void *)((uint8_t *)ah + sizeof(block_header_t));
}

/// Check the next block adjacent to 'b'. If it's free, return a pointer to
/// it.
static fblock_header_t *
next_fblock_adj(heap_t *heap, block_header_t *b)
{
    uint8_t *term = (uint8_t *)heap->vaddr + heap->pages * PAGE_SIZE;
    uint8_t *ptr  = (uint8_t *)b + b->size + sizeof(block_header_t) +
                    sizeof(block_footer_t);
    if (ptr >= term)
        return NULL;

    block_header_t *h = (block_header_t *)ptr;
    if ((h->flags & FLAG_ALLOCATED) == 0)
        return (fblock_header_t *)h;

    return NULL;
}

/// Check the preceding block adjacent to 'b'. If it's free, return a pointer
/// to it.
static fblock_header_t *
prev_fblock_adj(heap_t *heap, block_header_t *b)
{
    uint8_t *start = (uint8_t *)heap->vaddr + sizeof(heap_t);
    if ((uint8_t *)b == start)
        return NULL;

    block_footer_t *f =
        (block_footer_t *)((uint8_t *)b - sizeof(block_footer_t));
    block_header_t *h =
        (block_header_t *)((uint8_t *)b - f->size - sizeof(block_header_t) -
                           sizeof(block_footer_t));
    if ((h->flags & FLAG_ALLOCATED) == 0)
        return (fblock_header_t *)h;

    return NULL;
}

/// Scan all blocks after 'b' until a free block is encountered and return it.
/// If no free blocks are found, return NULL.
static fblock_header_t *
next_fblock(heap_t *heap, block_header_t *b)
{
    uint8_t *term = (uint8_t *)heap->vaddr + heap->pages * PAGE_SIZE;
    for (;;) {
        uint8_t *ptr = (uint8_t *)b + b->size + sizeof(block_header_t) +
                       sizeof(block_footer_t);
        if (ptr >= term)
            return NULL;

        b = (block_header_t *)ptr;
        if ((b->flags & FLAG_ALLOCATED) == 0)
            return (fblock_header_t *)b;
    }
    return NULL;
}

/// Scan all blocks preceding 'b' until a free block is encountered and return
/// it. If no free blocks are found, return NULL.
static fblock_header_t *
prev_fblock(heap_t *heap, block_header_t *b)
{
    uint8_t *start = (uint8_t *)heap->vaddr + sizeof(heap_t);
    for (;;) {
        if ((uint8_t *)b == start)
            return NULL;

        block_footer_t *f =
            (block_footer_t *)((uint8_t *)b - sizeof(block_footer_t));
        b = (block_header_t *)((uint8_t *)b - f->size -
                               sizeof(block_header_t) -
                               sizeof(block_footer_t));
        if ((b->flags & FLAG_ALLOCATED) == 0)
            return (fblock_header_t *)b;
    }
    return NULL;
}

void
heap_free(heap_t *heap, void *ptr)
{
    // @@@ dummy code.  TODO: write free code.
    block_header_t *h =
        (block_header_t *)((uint8_t *)ptr - sizeof(block_header_t));
    fblock_header_t *f1 = next_fblock_adj(heap, h);
    fblock_header_t *f2 = prev_fblock_adj(heap, h);
    fblock_header_t *f3 = next_fblock(heap, h);
    fblock_header_t *f4 = prev_fblock(heap, h);
    (void)f1;
    (void)f2;
    (void)f3;
    (void)f4;
}
