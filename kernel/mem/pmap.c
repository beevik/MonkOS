//============================================================================
/// @file       pmap.c
/// @brief      Physical memory map describing usable and reserved regions
///             of physical memory.
/// @details    Before this code is ever executed, the boot code has filled
///             much of the memory map with memory regions supplied by the
///             BIOS.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#include <core.h>
#include <libc/stdlib.h>
#include <libc/string.h>
#include <kernel/mem/pmap.h>
#include "kmem.h"

// Pointer to the BIOS-generated memory map.
static pmap_t *map         = (pmap_t *)KMEM_TABLE_BIOS;
static bool    initialized = false;

/// Add a memory region to the end of the memory map.
static void
add_region(uint64_t addr, uint64_t size, enum pmemtype type)
{
    pmapregion_t *r = map->region + map->count;
    r->addr  = addr;
    r->size  = size;
    r->type  = (int32_t)type;
    r->flags = 0;

    ++map->count;
}

/// Compare two memory region records and return a sorting integer.
static int
cmp_region(const void *a, const void *b)
{
    const pmapregion_t *r1 = (const pmapregion_t *)a;
    const pmapregion_t *r2 = (const pmapregion_t *)b;
    if (r1->addr > r2->addr)
        return +1;
    if (r1->addr < r2->addr)
        return -1;
    if (r1->size > r2->size)
        return +1;
    if (r1->size < r2->size)
        return -1;
    return 0;
}

/// Remove a region from the memory map and shift all subsequent regions
/// down by one.
static inline void
collapse(pmapregion_t *r, pmapregion_t *term)
{
    if (r + 1 < term)
        memmove(r, r + 1, (term - r) * sizeof(pmapregion_t));
    --map->count;
}

/// Insert a new, uninitialized memory region record after an existing record
/// in the memory map.
static inline void
insertafter(pmapregion_t *r, pmapregion_t *term)
{
    if (r + 1 < term)
        memmove(r + 2, r + 1, (term - (r + 1)) * sizeof(pmapregion_t));
    ++map->count;
}

/// Re-sort all unsorted region records starting from the requested record.
static void
resort(pmapregion_t *r, pmapregion_t *term)
{
    while (r + 1 < term) {
        if (cmp_region(r, r + 1) < 0)
            break;
        pmapregion_t tmp = r[0];
        r[0] = r[1];
        r[1] = tmp;
        r++;
    }
}

/// Find all overlapping memory regions in the memory map and collapse or
/// reorganize them.
static void
collapse_overlaps()
{
    pmapregion_t *curr = map->region;
    pmapregion_t *term = map->region + map->count;

    while (curr + 1 < term) {

        // Collapse empty entries.
        if (curr->size == 0) {
            collapse(curr, term--);
            continue;
        }

        pmapregion_t *next = curr + 1;

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
        //   xxx    xxx    xxxx   xxx    xxxxx
        //   yyy    yyyy    yyy    yyy    yyy
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
            else { /* if cr != nr */
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
        }

    }
}

/// Find missing memory regions in the map, and fill them with entries of
/// the requested type.
static void
fill_gaps(int32_t type)
{
    pmapregion_t *curr = map->region;
    pmapregion_t *term = map->region + map->count;

    while (curr + 1 < term) {
        pmapregion_t *next = curr + 1;

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
    pmapregion_t *curr = map->region;
    pmapregion_t *term = map->region + map->count;

    while (curr + 1 < term) {
        pmapregion_t *next = curr + 1;
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
update_last_usable()
{
    map->last_usable = 0;
    for (int i = map->count - 1; i >= 0; i--) {
        const pmapregion_t *r = &map->region[i];
        if (r->type == PMEMTYPE_USABLE) {
            map->last_usable = r->addr + r->size;
            break;
        }
    }
}

static void
normalize()
{
    // Sort the memory map by address.
    qsort(map->region, map->count, sizeof(pmapregion_t),
          cmp_region);

    // Remove overlapping regions, fill gaps between regions with "reserved"
    // memory, squash adjacent regions of the same type, and calculate the end
    // of the last usable memory region.
    collapse_overlaps();
    fill_gaps(PMEMTYPE_RESERVED);
    consolidate_neighbors();
    update_last_usable();
}

void
pmap_init()
{
    // During the boot process, the physical memory map at KMEM_TABLE_BIOS has
    // been updated to include memory regions reported by the BIOS. This
    // function cleans up the BIOS memory map (sorts it, removes overlaps,
    // etc.) and adds a few additional memory regions.

    // Mark VGA video memory as uncached.
    add_region(KMEM_VIDEO, KMEM_VIDEO_SIZE, PMEMTYPE_UNCACHED);

    // Reserve memory for the kernel and its global data structures.
    add_region(0, KMEM_KERNEL_IMAGE_END, PMEMTYPE_RESERVED);

    // Mark the first page of memory as unmapped so deferencing a null pointer
    // always faults.
    add_region(0, 0x1000, PMEMTYPE_UNMAPPED);

    // Fix up the memory map.
    normalize();

    initialized = true;
}

const pmap_t *
pmap()
{
    return map;
}

void
pmap_add(uint64_t addr, uint64_t size, enum pmemtype type)
{
    add_region(addr, size, type);

    if (initialized)
        normalize();
}
