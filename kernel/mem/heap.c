//============================================================================
/// @file       heap.c
/// @brief      A simple heap memory manager
//
//  Copyright 2016 Brett Vickers.
//  Use of this source code is governed by a BSD-style license
//  that can be found in the MonkOS LICENSE file.
//============================================================================

#include <core.h>
#include <libc/string.h>
#include <kernel/mem/heap.h>
#include <kernel/mem/paging.h>

// ALLOC_PAGES: The minimum number of pages to allocate each time the heap
// is grown.
#define ALLOC_PAGES     16

// block_header flags
#define FLAG_ALLOCATED  (1 << 0)

// round16 returns the first value x greater than or equal to n that satisfies
// (x mod 16) = r
#define round16(n, r)  (((((n) - (r) + 31) >> 4) << 4) + (r) - 16)

// total_bytes returns the total number of bytes contained in the block 'b',
// including the block header and footer.  'b' may be a pointer to a
// block_header or a block_footer struct.
#define total_bytes(b) \
    ((b)->size + sizeof(block_header_t) + sizeof(block_footer_t))

struct heap
{
    pagetable_t          *pt;           // page table that owns the heap
    void                 *vaddr;        // address of heap start
    uint64_t              pages;        // pages currently alloced to the heap
    uint64_t              maxpages;     // max pages used by the heap
    struct fblock_header *first_fblock; // first free block in the heap
    uint64_t              reserved;     // pad to a multiple of 16 bytes
};

typedef struct block_header
{
    uint64_t size;          // size of block in bytes (minus header/footer)
    uint64_t flags;
} block_header_t;

typedef struct block_footer
{
    uint64_t size;          // size of the preceding block
} block_footer_t;

typedef struct fblock_header
{
    struct block_header   block;
    struct fblock_header *next_fblock;  // next free block in the heap
    struct fblock_header *prev_fblock;  // prev free block in the heap
} fblock_header_t;

heap_t *
heap_create(pagetable_t *pt, void *vaddr, uint64_t maxpages)
{
    heap_t *heap = (heap_t *)page_alloc(pt, vaddr, ALLOC_PAGES);
    heap->pt           = pt;
    heap->vaddr        = vaddr;
    heap->pages        = ALLOC_PAGES;
    heap->maxpages     = max(ALLOC_PAGES, maxpages);
    heap->first_fblock = (fblock_header_t *)(heap + 1);
    heap->reserved     = 0;

    uint64_t block_size = heap->pages * PAGE_SIZE -
                          sizeof(heap_t) -
                          sizeof(block_header_t) -
                          sizeof(block_footer_t);

    fblock_header_t *fh = heap->first_fblock;
    fh->block.size  = block_size;
    fh->block.flags = 0;
    fh->next_fblock = NULL;
    fh->prev_fblock = NULL;

    block_footer_t *footer = ptr_add(block_footer_t, fh, block_size +
                                     sizeof(block_header_t));
    footer->size = block_size;

    return heap;
}

void
heap_destroy(heap_t *heap)
{
    page_free(heap->pt, heap->vaddr, heap->pages);
    // The heap pointer now points to unpaged memory.
}

/// Check the next block adjacent to 'b'. If it's free, return a pointer to
/// it.
static fblock_header_t *
next_fblock_adj(heap_t *heap, block_header_t *bh)
{
    block_header_t *term = ptr_add(block_header_t, heap->vaddr,
                                   heap->pages * PAGE_SIZE);
    block_header_t *next = ptr_add(block_header_t, bh, total_bytes(bh));
    if (next >= term)
        return NULL;
    if ((next->flags & FLAG_ALLOCATED) == 0)
        return (fblock_header_t *)next;
    return NULL;
}

/// Check the preceding block adjacent to 'b'. If it's free, return a pointer
/// to it.
static fblock_header_t *
prev_fblock_adj(heap_t *heap, block_header_t *bh)
{
    block_header_t *first = ptr_add(block_header_t, heap->vaddr,
                                    sizeof(heap_t));
    if (bh == first)
        return NULL;
    block_footer_t *bf = ptr_sub(block_footer_t, bh,
                                 sizeof(block_footer_t));
    block_header_t *prev = ptr_sub(block_header_t, bf, total_bytes(bf));
    if ((prev->flags & FLAG_ALLOCATED) == 0)
        return (fblock_header_t *)prev;
    return NULL;
}

/// Scan all blocks after 'b' until a free block is encountered and return it.
/// If no free blocks are found, return NULL.
static fblock_header_t *
next_fblock(heap_t *heap, block_header_t *bh)
{
    block_header_t *term = ptr_add(block_header_t, heap->vaddr,
                                   heap->pages * PAGE_SIZE);
    for (;;) {
        bh = ptr_add(block_header_t, bh, total_bytes(bh));
        if (bh >= term)
            return NULL;
        if ((bh->flags & FLAG_ALLOCATED) == 0)
            return (fblock_header_t *)bh;
    }
    return NULL;
}

/// Scan all blocks preceding 'b' until a free block is encountered and return
/// it. If no free blocks are found, return NULL.
static fblock_header_t *
prev_fblock(heap_t *heap, block_header_t *bh)
{
    block_header_t *first = ptr_add(block_header_t, heap->vaddr,
                                    sizeof(heap_t));
    for (;;) {
        if (bh == first)
            return NULL;
        block_footer_t *bf = ptr_sub(block_footer_t, bh,
                                     sizeof(block_footer_t));
        bh = ptr_sub(block_header_t, bh, total_bytes(bf));
        if ((bh->flags & FLAG_ALLOCATED) == 0)
            return (fblock_header_t *)bh;
    }
    return NULL;
}

/// Grow the heap so that it's big enough to hold at least minsize newly
/// allocated bytes (not including headers). Return a pointer to the
/// first new free block if successful. Otherwise return NULL.
static fblock_header_t *
grow_heap(heap_t *heap, uint64_t minsize)
{
    // Account for headers when growing the heap.
    minsize += sizeof(block_header_t) + sizeof(block_footer_t);
    uint64_t pages = max(ALLOC_PAGES, div_up(minsize, PAGE_SIZE));

    // Don't allocate more than maxpages to the heap.
    if (heap->pages + pages > heap->maxpages) {
        pages = heap->maxpages - heap->pages;
        if (pages * PAGE_SIZE < minsize)
            return NULL;
    }

    // Compute the virtual address of the next group of pages and allocate
    // them from the page table.
    void *vnext = ptr_add(void, heap->vaddr, heap->pages * PAGE_SIZE);
    page_alloc(heap->pt, vnext, pages);
    heap->pages += pages;

    // Examine the last block in the heap to see if it's free.
    block_footer_t *lf = ptr_sub(block_footer_t, vnext,
                                 sizeof(block_footer_t));
    block_header_t *lh = ptr_sub(block_header_t, lf, lf->size +
                                 sizeof(block_header_t));

    // If the last block in the old heap was in use, mark the newly allocated
    // pages as a single free block of memory.
    fblock_header_t *fh;
    if (lh->flags & FLAG_ALLOCATED) {
        fh             = (fblock_header_t *)vnext;
        fh->block.size = pages * PAGE_SIZE - sizeof(block_header_t) -
                         sizeof(block_footer_t);
        fh->block.flags = 0;
        fh->next_fblock = NULL;
        fh->prev_fblock = prev_fblock(heap, (block_header_t *)vnext);
        if (fh->prev_fblock == NULL)
            heap->first_fblock = fh;
    }

    // If the last block in the old heap was free, merge it with the newly
    // allocated pages.
    else {
        fh              = (fblock_header_t *)lh;
        fh->block.size += pages * PAGE_SIZE;
    }

    // Update the free block footer.
    block_footer_t *ff = ptr_add(block_footer_t, fh, fh->block.size +
                                 sizeof(block_header_t));
    ff->size = fh->block.size;

    return fh;
}

// Find a free block large enough to hold 'size' bytes. If such a block is not
// found, grow the heap and try again.
static fblock_header_t *
find_fblock(heap_t *heap, uint64_t size)
{
    fblock_header_t *fh = heap->first_fblock;
    while (fh) {
        if (fh->block.size >= size)
            return fh;
        fh = fh->next_fblock;
    }

    // No free blocks large enough were found, so grow the heap.
    return grow_heap(heap, size);
}

void *
heap_alloc(heap_t *heap, uint64_t size)
{
    // Round the size up to the nearest (mod 16) == 8 value. This way all
    // returned pointers will remain aligned on 16-byte boundaries.
    size = round16(size, 16 - sizeof(block_footer_t));

    // Find a free block big enough to hold the allocation
    fblock_header_t *fh = find_fblock(heap, size);
    if (fh == NULL)
        return NULL;

    // Copy the original free block's size and pointers, since we'll be
    // modifying their current storage.
    uint64_t         fsize = fh->block.size;
    fblock_header_t *next  = fh->next_fblock;
    fblock_header_t *prev  = fh->prev_fblock;

    // If the free block would be filled (or nearly filled) by the
    // allocation, replace it with an allocated block.
    block_header_t *ah;
    if (fsize - size < 8 + sizeof(block_header_t) + sizeof(block_footer_t)) {

        // Mark the block header as allocated, leaving the size unchanged.
        ah        = &fh->block;
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
        ah        = &fh->block;
        ah->size  = size;
        ah->flags = FLAG_ALLOCATED;

        // Initialize the allocated block footer.
        block_footer_t *af = ptr_add(block_footer_t, ah, size +
                                     sizeof(block_header_t));
        af->size = size;

        // Initialize the new free block header, which follows the allocated
        // block.
        fblock_header_t *fh = (fblock_header_t *)(af + 1);
        fh->block.size = fsize - size - sizeof(block_header_t) -
                         sizeof(block_footer_t);
        fh->block.flags = 0;

        // Initialize the new free block footer.
        block_footer_t *ff = ptr_add(block_footer_t, fh, fh->block.size +
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
    return ptr_add(void, ah, sizeof(block_header_t));
}

void
heap_free(heap_t *heap, void *ptr)
{
    block_header_t *h = ptr_sub(block_header_t, ptr, sizeof(block_header_t));

    // Check if adjacent blocks are free.
    fblock_header_t *fhp = prev_fblock_adj(heap, h);
    fblock_header_t *fhn = next_fblock_adj(heap, h);

    // If both adjacent blocks are free, merge all three into a single
    // free block.
    if (fhp && fhn) {
        fhp->block.size += h->size + 2 * sizeof(block_header_t) +
                           2 * sizeof(block_footer_t) + fhn->block.size;

        fhp->next_fblock = fhn->next_fblock;
        if (fhn->next_fblock != NULL)
            fhn->next_fblock->prev_fblock = fhp;

        block_footer_t *ffp = ptr_add(block_footer_t, fhp, fhp->block.size +
                                      sizeof(block_header_t));
        ffp->size = fhp->block.size;
    }

    // If only the previous adjacent block is free, merge it with the newly
    // free block.
    else if (fhp && !fhn) {
        fhp->block.size += total_bytes(h);
        block_footer_t *ffp = ptr_add(block_footer_t, fhp, fhp->block.size +
                                      sizeof(block_header_t));
        ffp->size = fhp->block.size;
    }

    // If only the next adjacent block is free, merge it with the newly free
    // block.
    else if (fhn && !fhp) {
        fblock_header_t *fh = (fblock_header_t *)h;
        fh->block.size += total_bytes(&fhn->block);
        fh->block.flags = 0;

        fh->prev_fblock = fhn->prev_fblock;
        if (fh->prev_fblock == NULL)
            heap->first_fblock = fh;
        else
            fh->prev_fblock->next_fblock = fh;

        fh->next_fblock = fhn->next_fblock;
        if (fh->next_fblock != NULL)
            fh->next_fblock->prev_fblock = fh;

        block_footer_t *ff = ptr_add(block_footer_t, fh, fh->block.size +
                                     sizeof(block_header_t));
        ff->size = fh->block.size;
    }

    // If neither adjacent block is free, convert the block into a single
    // free block.
    else {
        fblock_header_t *fh = (fblock_header_t *)h;
        fh->block.flags = 0;

        fh->next_fblock = next_fblock(heap, h);
        if (fh->next_fblock == NULL)
            fh->prev_fblock = prev_fblock(heap, h);
        else {
            fh->prev_fblock              = fh->next_fblock->prev_fblock;
            fh->next_fblock->prev_fblock = fh;
        }

        if (fh->prev_fblock == NULL)
            heap->first_fblock = fh;
        else
            fh->prev_fblock->next_fblock = fh;
    }
}
