//============================================================================
/// @file       paging.c
/// @brief      Paged memory management.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#include <core.h>
#include <libc/stdlib.h>
#include <libc/string.h>
#include <kernel/interrupt/interrupt.h>
#include <kernel/mem/table.h>
#include <kernel/mem/paging.h>
#include "map.h"

// Page shift constants
#define PAGE_SHIFT          12   // 1<<12 = 4KiB
#define PAGE_SHIFT_LARGE    21   // 1<<21 = 2MiB

// Page frame types
enum
{
    PFTYPE_RESERVED  = 0,
    PFTYPE_AVAILABLE = 1,
    PFTYPE_ALLOCATED = 2,
};

// Page frame number constants
#define PFN_INVALID    ((uint32_t)-1)

/// The pf structure represents a page frame record, which tracks page frames
/// in the page frame database.
typedef struct pf
{
    uint32_t prev;               ///< Prev pfn on available list
    uint32_t next;               ///< Next pfn on available list
    uint64_t ptaddr;             ///< Address of this pf's page table entry
    uint16_t refcount;           ///< Number of references to this page
    uint16_t sharecount;         ///< Number of processes sharing page
    uint16_t flags;
    uint8_t  type;               ///< PFTYPE of page frame
    uint8_t  reserved0;
    uint64_t reserved1;
} pf_t;

STATIC_ASSERT(sizeof(pf_t) == 32, "Unexpected page frame size");

/// The page union contains the contents of a 4KiB page, whether it's mapped
/// as a page table or a leaf page.
typedef union page
{
    uint64_t entry[512];         ///< Page table entries (if page is a table)
    uint8_t  memory[PAGE_SIZE];  ///< Memory contents (if page is a leaf)
} page_t;

STATIC_ASSERT(sizeof(page_t) == PAGE_SIZE, "Unexpected page size");

/// The state structure holds the global state of the paging module.
typedef struct state
{
    page_t  *pagetable;          ///< The kernel's page table
    pf_t    *pfdb;               ///< Page frame database
    uint32_t pfcount;            ///< Number of page frames in the pfdb.
    uint32_t pfavail;            ///< Available page frames in the pfdb;
    uint32_t pfnhead;            ///< Available page frame list head
    uint32_t pfntail;            ///< Available page frame list tail
} state_t;

static state_t state;

/// Reserve an aligned region of memory managed by the memory table module.
static void *
reserve_region(const memtable_t *table, uint64_t size, uint32_t alignshift)
{
    const memregion_t *r = table->region;
    const memregion_t *t = table->region + table->count;
    for (; r < t; r++) {

        // Skip reserved regions and regions that are too small.
        if (r->type != MEMTYPE_USABLE)
            continue;
        if (r->size < size)
            continue;

        // Find address of first properly aligned byte in the region.
        uint64_t addr = r->addr + (1 << alignshift) - 1;
        addr >>= alignshift;
        addr <<= alignshift;

        // If the region is too small after alignment, skip it.
        if (addr + size > r->addr + r->size)
            continue;

        // Reserve the aligned memory region and return its address.
        memtable_reserve(addr, size);
        return (void *)addr;
    }
    return NULL;
}

void
page_init()
{
    const memtable_t *table = memtable();

    // Find the last possible physical address in the memory table.
    const memregion_t *lastregion = &table->region[table->count - 1];
    uint64_t           lastaddr   = (lastregion->addr + lastregion->size);

    // The page frame database needs to describe each page up to and including
    // the last physical address.
    state.pfcount = lastaddr / PAGE_SIZE;
    uint64_t pfdbsize = state.pfcount * sizeof(pf_t);

    // Round the database size up to the nearest 2MiB since we'll be using
    // large pages in the kernel page table to describe the database.
    pfdbsize  += PAGE_SIZE_LARGE - 1;
    pfdbsize >>= PAGE_SHIFT_LARGE;
    pfdbsize <<= PAGE_SHIFT_LARGE;

    // Find a contiguous, 2MiB-aligned region of memory large enough to hold
    // the entire pagedb.
    state.pfdb = (pf_t *)reserve_region(table, pfdbsize, PAGE_SHIFT_LARGE);
    if (state.pfdb == NULL)
        fatal();

    // Identity-map all physical memory into the kernel's page table.
    map_memory();

    // Keep track of the root PML4 table.
    state.pagetable = (page_t *)MEM_KERNEL_PAGETABLE;

    // Create the page frame database in the newly mapped virtual memory.
    memzero(state.pfdb, pfdbsize);

    // Initialize available page frame list.
    state.pfavail = 0;
    state.pfnhead = PFN_INVALID;
    state.pfntail = PFN_INVALID;

    // Traverse the memory table, adding page frame database entries for each
    // region in the table.
    for (uint64_t r = 0; r < table->count; r++) {
        const memregion_t *region = &table->region[r];

        // Ignore non-usable memory regions.
        if (region->type != MEMTYPE_USABLE)
            continue;

        // Create a chain of available page frames.
        uint64_t pfn0 = region->addr >> PAGE_SHIFT;
        uint64_t pfnN = (region->addr + region->size) >> PAGE_SHIFT;
        for (uint64_t pfn = pfn0; pfn < pfnN; pfn++) {
            pf_t *pf = state.pfdb + pfn;
            pf->prev = pfn - 1;
            pf->next = pfn + 1;
            pf->type = PFTYPE_AVAILABLE;
        }

        // Link the chain to the list of available frames.
        if (state.pfntail == PFN_INVALID)
            state.pfnhead = pfn0;
        else
            state.pfdb[state.pfntail].next = pfn0;
        state.pfdb[pfn0].prev     = state.pfntail;
        state.pfdb[pfnN - 1].next = PFN_INVALID;
        state.pfntail             = pfnN - 1;

        // Update the total number of available frames.
        state.pfavail += (uint32_t)(pfnN - pfn0);
    }

    // Install page fault handler
}

pagetable_handle
pagetable_create()
{
    // Allocate and zero a single page from the page database for the PML4
    // table.

    // Return the PML4 address converted to a handle.

    return (pagetable_handle)0;
}

void
pagetable_activate(pagetable_handle pt)
{
    (void)pt;
    // Set the CR3 register to the page table's PML4 table address.
}

void
pagetable_destroy(pagetable_handle pt)
{
    (void)pt;

    // Traverse the pagetable's child tables and free each page encountered.
}

void *
page_alloc(pagetable_handle pt, int count, uint32_t flags)
{
    (void)pt;
    (void)count;
    (void)flags;

    // Claim a chain of 'count' page frames from the pf database.

    // For each claimed page:
    // - Look up the page's physical address in the page table.
    // - If necessary, claim page frames to hold the page table pages
    //   needed to reach the address.
    // - Update the page table entries needed to reach the address.

    // Return a pointer to the first claimed page.

    return NULL;
}

void
page_free(pagetable_handle pt, void *addr, int count)
{
    (void)pt;
    (void)addr;
    (void)count;

    // Iterate 'count' pages starting from 'addr'
    // - Look up the page's physical address using the page table.
    // - Add the page frame to a list of frames to be freed.
    // - Clear the page's entry in the page table.

    // Add the list of freed page frames back to the pf database.
}
