//============================================================================
/// @file       paging.c
/// @brief      Virtual memory paging.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#include <core.h>
#include <kernel/memlayout.h>
#include <kernel/paging.h>
#include <libc/stdlib.h>
#include <libc/string.h>

enum
{
    TYPE_USABLE   = 1,
    TYPE_RESERVED = 2,
    TYPE_ACPI     = 3,
    TYPE_ACPI_NVS = 4,
    TYPE_BAD      = 5,
};

typedef struct layoutentry
{
    uint64_t addr;
    uint64_t size;
    int32_t  type;
    uint32_t flags;
} layoutentry_t;

static uint64_t      *layoutsize = (uint64_t *)(MEM_LAYOUT_BIOS);
static layoutentry_t *layout     = (layoutentry_t *)(MEM_LAYOUT_BIOS + 16);

static void
add_entry(uint64_t addr, uint64_t size, int32_t type)
{
    layoutentry_t *e = layout + *layoutsize;
    e->addr  = addr;
    e->size  = size;
    e->type  = type;
    e->flags = 0;

    ++*layoutsize;
}

static int
cmp_layoutentry(const void *a, const void *b)
{
    const layoutentry_t *e1 = (const layoutentry_t *)a;
    const layoutentry_t *e2 = (const layoutentry_t *)b;
    if (e1->addr > e2->addr)
        return +1;
    else if (e1->addr < e2->addr)
        return -1;
    else if (e1->size > e2->size)
        return +1;
    else if (e1->size < e2->size)
        return -1;
    else
        return 0;
}

static inline void
collapse(layoutentry_t *e, layoutentry_t *term)
{
    if (e + 1 < term)
        memmove(e, e + 1, (term - e) * sizeof(layoutentry_t));
    --*layoutsize;
}

static inline void
insertafter(layoutentry_t *e, layoutentry_t *term)
{
    if (e + 1 < term)
        memmove(e + 2, e + 1, (term - (e + 1)) * sizeof(layoutentry_t));
    ++*layoutsize;
}

static void
resort(layoutentry_t *e, layoutentry_t *term)
{
    while (e + 1 < term) {
        if (cmp_layoutentry(e, e + 1) < 0)
            break;
        layoutentry_t tmp = e[0];
        e[0] = e[1];
        e[1] = tmp;
        e++;
    }
}

static void
collapse_overlaps()
{
    layoutentry_t *curr = layout;
    layoutentry_t *term = layout + *layoutsize;

    while (curr + 1 < term) {

        // Collapse empty entries.
        if (curr->size == 0) {
            collapse(curr, term--);
            continue;
        }

        layoutentry_t *next = curr + 1;

        uint64_t cl = curr->addr;
        uint64_t cr = curr->addr + curr->size;
        uint64_t nl = next->addr;
        uint64_t nr = next->addr + next->size;

        // No overlap? Then go to next entry.
        if (min(cr, nr) <= max(cl, nl)) {
            curr++;
            continue;
        }

        // Handle the 5 alignment cases:
        //   xxx   xxx   xxxx  xxx   xxxxx
        //   yyy   yyyy   yyy   yyy   yyy

        if (cl == nl) {
            if (cr == nr) {
                if (next->type > curr->type) {
                    // 111  ->  222
                    // 222
                    collapse(curr, term--);
                }
                else {
                    // 222  ->  222
                    // 111
                    collapse(next, term--);
                }
            }
            else { /* cr != nr */
                if (next->type > curr->type) {
                    // 111  ->  2222
                    // 2222
                    collapse(curr, term--);
                }
                else {
                    // 222  ->  222
                    // 1111 ->     1
                    next->size = nr - cr;
                    next->addr = cr;
                    resort(next, term);
                }
            }
        }

        else { /* if cl != nl */
            if (cr == nr) {
                if (next->type > curr->type) {
                    // 1111  ->  1
                    //  222  ->   222
                    curr->size = nl - cl;
                }
                else {
                    // 2222  ->  2222
                    //  111
                    collapse(next, term--);
                }
            }
            else if (cr < nr) {
                if (next->type > curr->type) {
                    // 1111  ->  1
                    //  2222 ->   2222
                    curr->size = nl - cl;
                }
                else {
                    // 2222  ->  2222
                    //  1111 ->      1
                    next->size = nr - cr;
                    next->addr = cr;
                    resort(next, term);
                }
            }
            else { /* if cr > nr */
                if (next->type > curr->type) {
                    // 11111  -> 1
                    //  222   ->  222
                    //        ->     1
                    curr->size = nl - cl;
                    insertafter(next, term++);
                    next[1].addr  = nr;
                    next[1].size  = cr - nr;
                    next[1].type  = curr->type;
                    next[1].flags = curr->flags;
                    resort(next + 1, term);
                }
                else {
                    // 22222  ->  22222
                    //  111
                    collapse(next, term--);
                }
            }
        } // cl != nl

    } // while (curr + 1 < term)
}

static void
fill_gaps(int32_t type)
{
    layoutentry_t *curr = layout;
    layoutentry_t *term = layout + *layoutsize;

    while (curr + 1 < term) {
        layoutentry_t *next = curr + 1;

        uint64_t cr = curr->addr + curr->size;
        uint64_t nl = next->addr;

        if (cr == nl) {
            curr++;
            continue;
        }

        // Try to expand one of the neighboring entries if one of them has the
        // same type as the fill type.
        if (curr->type == type) {
            curr->size += nl - cr;
        }
        else if (next->type == type) {
            next->size += nl - cr;
            next->addr  = cr;
        }

        // Neither neighboring entry has the fill type, so insert a new entry.
        else {
            insertafter(curr, term++);
            next->addr  = cr;
            next->size  = nl - cr;
            next->type  = type;
            next->flags = 0;
        }
    }
}

static void
consolidate_neighbors()
{
    layoutentry_t *curr = layout;
    layoutentry_t *term = layout + *layoutsize;

    while (curr + 1 < term) {
        layoutentry_t *next = curr + 1;
        if (curr->type == next->type) {
            curr->size += next->size;
            collapse(next, term--);
        }
        else {
            curr++;
        }
    }
}

static void
remove_trailing(int32_t type)
{
    layoutentry_t *curr = layout + *layoutsize - 1;
    while (curr >= layout && curr->type == type) {
        --*layoutsize;
        --curr;
    }
}

static void
init_layout()
{
    // Reserve memory that the BIOS may not be aware of.
    add_entry(0, 1024 * 1024, TYPE_RESERVED);
    add_entry(MEM_KERNEL_IMAGE, MEM_KERNEL_IMAGE_SIZE, TYPE_RESERVED);
    add_entry(MEM_STACK_INTERRUPT_BOTTOM,
              MEM_STACK_INTERRUPT_TOP - MEM_STACK_INTERRUPT_BOTTOM,
              TYPE_RESERVED);
    add_entry(MEM_STACK_KERNEL_BOTTOM,
              MEM_STACK_KERNEL_TOP - MEM_STACK_KERNEL_BOTTOM,
              TYPE_RESERVED);

    // Sort the memory layout by address.
    qsort(layout, *layoutsize, sizeof(layoutentry_t), cmp_layoutentry);

    // Get rid of overlapping entries, fill gaps between entries with
    // "reserved" entries, consolidate adjacent entries of the same
    // type, and remove trailing reserved entries.
    collapse_overlaps();
    fill_gaps(TYPE_RESERVED);
    consolidate_neighbors();
    remove_trailing(TYPE_RESERVED);
}

void
page_init()
{
    init_layout();

    // Process BIOS memory layout (sort, fill gaps, fix up)

    // Calculate total available memory

    // Find a contiguous region of memory large enough for the pagedb

    // Build a kernel page table (rebuilding boot-loader-installed table)

    // Install page fault handler
}
