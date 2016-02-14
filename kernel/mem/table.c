//============================================================================
/// @file       table.c
/// @brief      Physical memory table describing usable and reserved regions
///             of physical memory.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#include <core.h>
#include <libc/stdlib.h>
#include <libc/string.h>
#include <kernel/mem/table.h>

// Physical memory layout supplied by the boot loader
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
#define MEM_PAGETABLE_PT              0x00013000
#define MEM_TABLE_BIOS                0x00018000
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

// Pointer to the BIOS-generated memory table table.
static memtable_t *table = (memtable_t *)MEM_TABLE_BIOS;

/// Add a memory region to the end of the memory table.
static void
add_region(uint64_t addr, uint64_t size, int32_t type)
{
    memregion_t *r = table->region + table->count;
    r->addr  = addr;
    r->size  = size;
    r->type  = type;
    r->flags = 0;

    ++table->count;
}

/// Compare two memory region records and return a sorting integer.
static int
cmp_region(const void *a, const void *b)
{
    const memregion_t *r1 = (const memregion_t *)a;
    const memregion_t *r2 = (const memregion_t *)b;
    if (r1->addr > r2->addr)
        return +1;
    else if (r1->addr < r2->addr)
        return -1;
    else if (r1->size > r2->size)
        return +1;
    else if (r1->size < r2->size)
        return -1;
    else
        return 0;
}

/// Remove a region from the memory table and shift all subsequent regions
/// down by one.
static inline void
collapse(memregion_t *r, memregion_t *term)
{
    if (r + 1 < term)
        memmove(r, r + 1, (term - r) * sizeof(memregion_t));
    --table->count;
}

/// Insert a new, uninitialized memory region record after an existing record
/// in the memory table.
static inline void
insertafter(memregion_t *r, memregion_t *term)
{
    if (r + 1 < term)
        memmove(r + 2, r + 1, (term - (r + 1)) * sizeof(memregion_t));
    ++table->count;
}

/// Re-sort all unsorted region records starting from the requested record.
static void
resort(memregion_t *r, memregion_t *term)
{
    while (r + 1 < term) {
        if (cmp_region(r, r + 1) < 0)
            break;
        memregion_t tmp = r[0];
        r[0] = r[1];
        r[1] = tmp;
        r++;
    }
}

/// Find all overlapping memory regions in the memory table and collapse or
/// reorganize them.
static void
collapse_overlaps()
{
    memregion_t *curr = table->region;
    memregion_t *term = table->region + table->count;

    while (curr + 1 < term) {

        // Collapse empty entries.
        if (curr->size == 0) {
            collapse(curr, term--);
            continue;
        }

        memregion_t *next = curr + 1;

        uint64_t cl = curr->addr;
        uint64_t cr = curr->addr + curr->size;
        uint64_t nl = next->addr;
        uint64_t nr = next->addr + next->size;

        // No overlap? Then go to the next region.
        if (min(cr, nr) <= max(cl, nl)) {
            curr++;
            continue;
        }

        // Handle the 5 alignment cases:
        //   xxx   xxx   xxxx  xxx   xxxxx
        //   yyy   yyyy   yyy   yyy   yyy
        // The remaining cases are impossible due to sorting.

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
            else {               /* cr != nr */
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

        else {                   /* if cl != nl */
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
            else {               /* if cr > nr */
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
        }

    }
}

/// Find missing memory regions in the table table, and fill them with
/// entries of the requested type.
static void
fill_gaps(int32_t type)
{
    memregion_t *curr = table->region;
    memregion_t *term = table->region + table->count;

    while (curr + 1 < term) {
        memregion_t *next = curr + 1;

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

        // Neither neighboring region has the fill type, so insert a new
        // region record.
        else {
            insertafter(curr, term++);
            next->addr  = cr;
            next->size  = nl - cr;
            next->type  = type;
            next->flags = 0;
        }
    }
}

/// Find adjacent memory entries of the same type and merge them.
static void
consolidate_neighbors()
{
    memregion_t *curr = table->region;
    memregion_t *term = table->region + table->count;

    while (curr + 1 < term) {
        memregion_t *next = curr + 1;
        if (curr->type == next->type) {
            curr->size += next->size;
            collapse(next, term--);
        }
        else {
            curr++;
        }
    }
}

/// Remove entries from the end of the memory table table if they are of the
/// requested type.
static void
remove_trailing(int32_t type)
{
    memregion_t *curr = table->region + table->count - 1;
    while (curr >= table->region && curr->type == type) {
        --table->count;
        --curr;
    }
}

void
memtable_init()
{
    // Reserve the first 10MiB of memory for the kernel and its global
    // data structures.
    add_region(0, MEM_KERNEL_IMAGE_END, MEMTYPE_RESERVED);

    // Sort the memory table by address.
    qsort(table->region, table->count, sizeof(memregion_t),
          cmp_region);

    // Get rid of overlapping entries, fill gaps between entries with
    // "reserved" entries, consolidate adjacent entries of the same
    // type, and remove trailing reserved entries.
    collapse_overlaps();
    fill_gaps(MEMTYPE_RESERVED);
    consolidate_neighbors();
    remove_trailing(MEMTYPE_RESERVED);
}

const memtable_t *
memtable()
{
    return table;
}
