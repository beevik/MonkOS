//============================================================================
/// @file       paging.c
/// @brief      Page frame database.
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
#include "memmap.h"

// PML4 table entry flags
#define PML4E_PRESENT       (1 << 0)
#define PML4E_RW            (1 << 1)
#define PML4E_USER          (1 << 2)
#define PML4E_PWT           (1 << 3) // Page write-thru
#define PML4E_PCD           (1 << 4) // Cache disable
#define PML4E_ACCESS        (1 << 5)

// Page directory pointer table entry flags
#define PDPTE_PRESENT       (1 << 0)
#define PDPTE_RW            (1 << 1)
#define PDPTE_USER          (1 << 2)
#define PDPTE_PWT           (1 << 3) // Page write-thru
#define PDPTE_PCD           (1 << 4) // Cache disable
#define PDPTE_ACCESS        (1 << 5)
#define PDPTE_DIRTY         (1 << 6)
#define PDPTE_PS            (1 << 7) // Page size (1 = 1GiB)
#define PDPTE_GLOBAL        (1 << 8)
#define PDPTE_PAT           (1 << 12) // Page attribute table value

// Page directory entry flags
#define PDE_PRESENT         (1 << 0)
#define PDE_RW              (1 << 1)
#define PDE_USER            (1 << 2)
#define PDE_PWT             (1 << 3) // Page write-thru
#define PDE_PCD             (1 << 4) // Cache disable
#define PDE_ACCESS          (1 << 5)
#define PDE_DIRTY           (1 << 6)
#define PDE_PS              (1 << 7) // Page size (1 = 2MiB)
#define PDE_GLOBAL          (1 << 8)
#define PDE_PAT             (1 << 12) // Page attribute table value

// Page table entry flags
#define PTE_PRESENT         (1 << 0)
#define PTE_RW              (1 << 1)
#define PTE_USER            (1 << 2)
#define PTE_PWT             (1 << 3) // Page write-thru
#define PTE_PCD             (1 << 4) // Cache disable
#define PTE_ACCESS          (1 << 5)
#define PTE_DIRTY           (1 << 6)
#define PTE_PAT             (1 << 7) // Page attribute table value
#define PTE_GLOBAL          (1 << 8)

// Virtual address subfield accessors
#define SHIFT_PML4E         39
#define SHIFT_PDPTE         30
#define SHIFT_PDE           21
#define SHIFT_PTE           12
#define PAGE_ENTRY_MASK     0x1ff

// Large page (2MiB) constants
#define LARGE_PAGE_SIZE     (1 << SHIFT_PDE) // 2MiB
#define LARGE_PAGE_SHIFT    SHIFT_PDE // 1<<21 == 2MiB

/// The pf structure represents a page frame record, which tracks page frames
/// in the page frame database.
typedef struct pf
{
    uint32_t next;
    uint32_t prev;
    uint64_t vaddr;
    uint16_t refcount;
    uint16_t sharecount;
    uint32_t flags;
    uint64_t reserved;
} pf_t;

/// The page union contains the contents of a 4KiB page, whether it's mapped
/// as a page table or a leaf page.
typedef union page
{
    uint64_t entry[512];         ///< Page table entries (if page is a table)
    uint8_t  memory[4096];       ///< Memory contents (if page is a leaf)
} page_t;

/// The state structure holds the global state of the paging module.
typedef struct state
{
    const memtable_t *memtable;  ///< Pointer to memory table
    uint64_t          pfdbsize;  ///< Size of pfdb in bytes
    pf_t             *pfdb;      ///< Page frame database
    uint64_t          pfavail;   ///< Total available physical pages
    page_t           *pagetable; ///< The kernel's page table
} state_t;

static state_t state;

/// Reserve an aligned region of memory managed by the memory table module.
static void *
reserve_region(uint64_t size, uint32_t alignshift)
{
    const memregion_t *r = state.memtable->region;
    const memregion_t *t = state.memtable->region + state.memtable->count;
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

static void
fatal()
{
    // Temporary. For now, generate a software interrupt to lock up the
    // machine.
    RAISE_INTERRUPT(0xff);
}

void
pagedb_init()
{
    // Calculate total available memory
    state.memtable = memtable();
    for (unsigned i = 0; i < state.memtable->count; i++) {
        if (state.memtable->region[i].type == MEMTYPE_USABLE)
            state.pfavail += state.memtable->region[i].size / PAGE_SIZE;
    }

    // Find the last possible physical address in the memory table.
    const memregion_t *lastregion =
        &state.memtable->region[state.memtable->count - 1];
    uint64_t lastaddr = (lastregion->addr + lastregion->size);

    // The page frame database needs to describe each page up to and including
    // the last physical address.
    state.pfdbsize = (lastaddr / PAGE_SIZE) * sizeof(pf_t);

    // Round up the database size to the nearest 2MiB since we'll be using
    // large pages in the kernel page table to describe the database.
    state.pfdbsize   = state.pfdbsize + (1 << LARGE_PAGE_SHIFT) - 1;
    state.pfdbsize >>= LARGE_PAGE_SHIFT;
    state.pfdbsize <<= LARGE_PAGE_SHIFT;

    // Find a contiguous region of memory large enough to hold the entire
    // pagedb.
    state.pfdb = (pf_t *)reserve_region(state.pfdbsize, LARGE_PAGE_SHIFT);
    if (state.pfdb == NULL)
        fatal();

    // Add the page frame directory to the page table installed by the boot
    // loader.
    uint64_t addr       = (uint64_t)state.pfdb;
    uint64_t term       = (uint64_t)state.pfdb + state.pfdbsize;
    page_t  *pdpt       = (page_t *)MEM_PAGETABLE_PDPT0;
    page_t  *curr_page  = (page_t *)MEM_PAGETABLE_PDT0;
    page_t  *next_page  = (page_t *)(MEM_PAGETABLE_PT0 + PAGE_SIZE);
    uint64_t curr_pdpte = (addr >> SHIFT_PDPTE) & PAGE_ENTRY_MASK;
    uint64_t curr_pde   = (addr >> SHIFT_PDE) & PAGE_ENTRY_MASK;
    for (; addr < term; addr += LARGE_PAGE_SIZE) {

        // Check if the page directory pointer table entry changed. If so,
        // we need to start another page directory page.
        uint64_t pdpte = (addr >> SHIFT_PDPTE) & PAGE_ENTRY_MASK;
        if (pdpte > curr_pdpte) {
            curr_page = next_page++;
            curr_pde  = 0;       // Reset page directory counter
            curr_pdpte++;        // Advance to the next PDPT entry
            pdpt->entry[curr_pdpte] =
                (uint64_t)curr_page | PDPTE_PRESENT | PDPTE_RW;
        }

        // Create a page table entry for a 2MiB page.
        curr_page->entry[curr_pde] = addr | PDE_PRESENT | PDE_RW | PDE_PS;
        ++curr_pde;
    }

    // Create the page frame database in the newly mapped virtual memory.

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
