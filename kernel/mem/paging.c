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
#include <kernel/x86/cpu.h>
#include <kernel/interrupt/interrupt.h>
#include <kernel/mem/kmem.h>
#include <kernel/mem/pmap.h>
#include <kernel/mem/paging.h>

// add_pte addflags
#define CONTAINS_TABLE     (1 << 0)

// Page shift constants
#define PAGE_SHIFT         12       // 1<<12 = 4KiB
#define PAGE_SHIFT_LARGE   21       // 1<<21 = 2MiB

// Page frame number constants
#define PFN_INVALID        ((uint32_t)-1)

// Helper macros
#define PADDR_TO_PF(a)     ((pf_t *)(pfdb.pf + ((a) >> PAGE_SHIFT)))
#define PF_TO_PADDR(pf)    ((uint64_t)((pf) - pfdb.pf) << PAGE_SHIFT)
#define PFN_TO_PF(pfn)     ((pf_t *)((pfn) + pfdb.pf))
#define PF_TO_PFN(pf)      ((uint32_t)((pf) - pfdb.pf))
#define PTE_TO_PADDR(pte)  ((pte) & ~PGMASK_OFFSET)

// Page frame types
enum
{
    PFTYPE_RESERVED  = 0,
    PFTYPE_AVAILABLE = 1,
    PFTYPE_ALLOCATED = 2,
};

/// The pf structure represents a page frame record, which tracks page frames
/// in the page frame database.
typedef struct pf
{
    uint32_t prev;          ///< Index of prev pfn on available list
    uint32_t next;          ///< Index of next pfn on available list
    uint16_t refcount;      ///< Number of references to this page
    uint16_t sharecount;    ///< Number of processes sharing page
    uint16_t flags;
    uint8_t  type;          ///< PFTYPE of page frame
    uint8_t  reserved0;
    uint64_t reserved1;
    uint64_t reserved2;
} pf_t;

STATIC_ASSERT(sizeof(pf_t) == 32, "Unexpected page frame size");

/// The pfdb describes the state of the page frame database.
struct pfdb
{
    pf_t    *pf;          ///< Pointer to array of page frames
    uint32_t count;       ///< Total number of frames in the pfdb
    uint32_t avail;       ///< Available number of frames in the pfdb
    uint32_t head;        ///< Index of available frame list head
    uint32_t tail;        ///< Index of available frame list tail
};

static struct pfdb pfdb;

/// Reserve an aligned region of memory managed by the memory table module.
static void *
reserve_region(const pmap_t *map, uint64_t size, uint32_t alignshift)
{
    const pmapregion_t *r = map->region;
    const pmapregion_t *t = map->region + map->count;
    for (; r < t; r++) {

        // Skip reserved regions and regions that are too small.
        if (r->type != PMEMTYPE_USABLE)
            continue;
        if (r->size < size)
            continue;

        // Find address of first properly aligned byte in the region.
        uint64_t paddr = r->addr + (1 << alignshift) - 1;
        paddr >>= alignshift;
        paddr <<= alignshift;

        // If the region is too small after alignment, skip it.
        if (paddr + size > r->addr + r->size)
            continue;

        // Reserve the aligned memory region and return its address.
        pmap_add(paddr, size, PMEMTYPE_RESERVED);
        return (void *)paddr;
    }
    return NULL;
}

void
page_init()
{
    // Retrieve the physical memory map.
    const pmap_t *map = pmap();
    if (map->last_usable == 0)
        fatal();

    // Calculate the size of the page frame database. It needs to describe
    // each page up to and including the last physical address.
    pfdb.count = map->last_usable / PAGE_SIZE;
    uint64_t pfdbsize = pfdb.count * sizeof(pf_t);

    // Round the database size up to the nearest 2MiB since we'll be using
    // large pages in the kernel page table to describe the database.
    pfdbsize  += PAGE_SIZE_LARGE - 1;
    pfdbsize >>= PAGE_SHIFT_LARGE;
    pfdbsize <<= PAGE_SHIFT_LARGE;

    // Find a contiguous, 2MiB-aligned region of memory large enough to hold
    // the entire pagedb.
    pfdb.pf = (pf_t *)reserve_region(map, pfdbsize, PAGE_SHIFT_LARGE);
    if (pfdb.pf == NULL)
        fatal();

    // Initialize the kernel's page table.
    uint64_t ptaddr = kmem_init();
    set_pagetable(ptaddr);

    // Create the page frame database in the newly mapped virtual memory.
    memzero(pfdb.pf, pfdbsize);

    // Initialize available page frame list.
    pfdb.avail = 0;
    pfdb.head  = PFN_INVALID;
    pfdb.tail  = PFN_INVALID;

    // Traverse the memory table, adding page frame database entries for each
    // region in the table.
    for (uint64_t r = 0; r < map->count; r++) {
        const pmapregion_t *region = &map->region[r];

        // Ignore non-usable memory regions.
        if (region->type != PMEMTYPE_USABLE)
            continue;

        // Create a chain of available page frames.
        uint64_t pfn0 = region->addr >> PAGE_SHIFT;
        uint64_t pfnN = (region->addr + region->size) >> PAGE_SHIFT;
        for (uint64_t pfn = pfn0; pfn < pfnN; pfn++) {
            pf_t *pf = PFN_TO_PF(pfn);
            pf->prev = pfn - 1;
            pf->next = pfn + 1;
            pf->type = PFTYPE_AVAILABLE;
        }

        // Link the chain to the list of available frames.
        if (pfdb.tail == PFN_INVALID)
            pfdb.head = pfn0;
        else
            pfdb.pf[pfdb.tail].next = pfn0;
        pfdb.pf[pfn0].prev     = pfdb.tail;
        pfdb.pf[pfnN - 1].next = PFN_INVALID;
        pfdb.tail              = pfnN - 1;

        // Update the total number of available frames.
        pfdb.avail += (uint32_t)(pfnN - pfn0);
    }

    // TODO: Install page fault handler
}

static pf_t *
pfalloc()
{
    // For now, fatal out. Later, we'll add swapping.
    if (pfdb.avail == 0)
        fatal();

    // Grab the first available page frame from the database.
    pf_t *pf = PFN_TO_PF(pfdb.head);

    // Update the available frame list.
    pfdb.head = pfdb.pf[pfdb.head].next;
    if (pfdb.head != PFN_INVALID)
        pfdb.pf[pfdb.head].prev = PFN_INVALID;

    // Initialize and return the page frame.
    memzero(pf, sizeof(pf_t));
    pf->refcount = 1;
    pf->type     = PFTYPE_ALLOCATED;
    return pf;
}

static void
pffree(pf_t *pf)
{
    if (pf->type != PFTYPE_ALLOCATED)
        fatal();

    // Re-initialize the page frame record.
    memzero(pf, sizeof(pf_t));
    pf->prev = PFN_INVALID;
    pf->next = pfdb.head;
    pf->type = PFTYPE_AVAILABLE;

    // Update the database's available frame list.
    uint32_t pfn = PF_TO_PFN(pf);
    pfdb.pf[pfdb.head].prev = pfn;
    pfdb.head               = pfn;

    pfdb.avail++;
}

static uint64_t
pgalloc()
{
    // Allocate a page frame from the db and compute its physical address.
    pf_t    *pf    = pfalloc();
    uint64_t paddr = PF_TO_PADDR(pf);

    // Always zero the contents of newly allocated pages.
    memzero((void *)paddr, PAGE_SIZE);

    // Return the page's physical address.
    return paddr;
}

static void
pgfree(uint64_t paddr)
{
    pf_t *pf = PADDR_TO_PF(paddr);
    if (--pf->refcount == 0)
        pffree(pf);
}

static void
pgfree_recurse(page_t *page, int level)
{
    // If we're at the PT level, return the leaf pages to the page frame
    // database.
    if (level == 1) {
        for (uint64_t e = 0; e < 512; e++) {
            uint64_t paddr = PTE_TO_PADDR(page->entry[e]);
            if (paddr == 0)
                continue;

            pf_t *pf = PADDR_TO_PF(paddr);
            if (pf->type == PFTYPE_ALLOCATED)
                pgfree(paddr);
        }
    }

    // If we're at the PD level or above, recursively traverse child tables
    // until we reach the PT level.
    else {
        for (uint64_t e = 0; e < 512; e++) {
            if (page->entry[e] & PF_SYSTEM)     // Never free system tables
                continue;
            page_t *child = PGPTR(page->entry[e]);
            if (child == NULL)
                continue;
            pgfree_recurse(child, level - 1);
        }
    }
}

/// Add to the page table an entry mapping a virtual address to a physical
/// address.
static void
add_pte(pagetable_t *pt, uint64_t vaddr, uint64_t paddr, uint32_t pflags,
        uint32_t addflags)
{
    // Fatal out if virtual address space is exhausted.
    if ((addflags & CONTAINS_TABLE) && (vaddr >= pt->vterm))
        fatal();

    // Keep track of the pages we add to the page table as we add this new
    // page table entry.
    uint64_t added[3];
    int      count = 0;

    // Decompose the virtual address into its hierarchical table components.
    uint32_t pml4e = PML4E(vaddr);
    uint32_t pdpte = PDPTE(vaddr);
    uint32_t pde   = PDE(vaddr);
    uint32_t pte   = PTE(vaddr);

    // Follow the page table hierarchy down to the lowest level, creating
    // new pages for tables as needed.

    page_t *pml4t = (page_t *)pt->proot;
    if (pml4t->entry[pml4e] == 0) {
        uint64_t pgaddr = pgalloc();
        added[count++]      = pgaddr;
        pml4t->entry[pml4e] = pgaddr | PF_PRESENT | PF_RW;
    }
    else if (pml4t->entry[pml4e] & PF_SYSTEM) {
        // A system page table should never be modified. This check is
        // performed only within the root PML4 table because checks on
        // lower level tables would be redundant.
        fatal();
    }

    page_t *pdpt = PGPTR(pml4t->entry[pml4e]);
    if (pdpt->entry[pdpte] == 0) {
        uint64_t pgaddr = pgalloc();
        added[count++]     = pgaddr;
        pdpt->entry[pdpte] = pgaddr | PF_PRESENT | PF_RW;
    }

    page_t *pdt = PGPTR(pdpt->entry[pdpte]);
    if (pdt->entry[pde] == 0) {
        uint64_t pgaddr = pgalloc();
        added[count++]  = pgaddr;
        pdt->entry[pde] = pgaddr | PF_PRESENT | PF_RW;
    }

    // Add the page table entry.
    page_t *ptt = PGPTR(pdt->entry[pde]);
    ptt->entry[pte] = paddr | pflags;

    // If adding the new entry required the page table to grow, make sure to
    // add the page table's new pages as well.
    for (int i = 0; i < count; i++) {
        add_pte(pt, pt->vnext, added[i], PF_PRESENT | PF_RW, CONTAINS_TABLE);
        pt->vnext += PAGE_SIZE;
    }
}

static uint64_t
remove_pte(pagetable_t *pt, uint64_t vaddr)
{
    // Decompose the virtual address into its hierarchical table components.
    uint32_t pml4e = PML4E(vaddr);
    uint32_t pdpte = PDPTE(vaddr);
    uint32_t pde   = PDE(vaddr);
    uint32_t pte   = PTE(vaddr);

    // Traverse the hierarchy, looking for the page.
    page_t *pml4t = (page_t *)pt->proot;
    page_t *pdpt  = PGPTR(pml4t->entry[pml4e]);
    page_t *pdt   = PGPTR(pdpt->entry[pdpte]);
    page_t *ptt   = PGPTR(pdt->entry[pde]);
    page_t *pg    = PGPTR(ptt->entry[pte]);

    // Clear the page table entry for the virtual address.
    ptt->entry[pte] = 0;

    // Return the physical address of the page table entry that was removed.
    return (uint64_t)pg;
}

void
pagetable_create(pagetable_t *pt, void *vaddr, uint64_t size)
{
    if (size % PAGE_SIZE != 0)
        fatal();

    // Allocate a page from the top level of the page table hierarchy.
    pt->proot = pgalloc();
    pt->vroot = (uint64_t)vaddr;
    pt->vnext = (uint64_t)vaddr + PAGE_SIZE;
    pt->vterm = (uint64_t)vaddr + size;

    // Install the kernel's page table into the created page table.
    page_t *src = (page_t *)kmem_pagetable_addr();
    page_t *dst = (page_t *)pt->proot;
    for (int i = 0; i < 512; i++)
        dst->entry[i] = src->entry[i];
}

void
pagetable_destroy(pagetable_t *pt)
{
    if (pt->proot == 0)
        fatal();

    // Recursively destroy all pages starting from the PML4 table.
    pgfree_recurse((page_t *)pt->proot, 4);
    pt->proot = 0;
    pt->vroot = 0;
    pt->vnext = 0;
    pt->vterm = 0;
}

void
pagetable_activate(pagetable_t *pt)
{
    if (pt == NULL) {
        uint64_t ptaddr = kmem_pagetable_addr();
        set_pagetable(ptaddr);
        return;
    }

    if (pt->proot == 0)
        fatal();

    set_pagetable(pt->proot);
}

void *
page_alloc(pagetable_t *pt, void *vaddr_in, int count)
{
    for (uint64_t vaddr = (uint64_t)vaddr_in; count--; vaddr += PAGE_SIZE) {
        uint64_t paddr = pgalloc();
        add_pte(pt, vaddr, paddr, PF_PRESENT | PF_RW, 0);
    }
    return vaddr_in;
}

void
page_free(pagetable_t *pt, void *vaddr_in, int count)
{
    for (uint64_t vaddr = (uint64_t)vaddr_in; count--; vaddr += PAGE_SIZE) {
        uint64_t paddr = remove_pte(pt, vaddr);
        pgfree(paddr);
    }
}
