//============================================================================
/// @file       kmem.c
/// @brief      Kernel physical (and virtual) memory map.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#include <core.h>
#include <libc/string.h>
#include <kernel/x86/cpu.h>
#include <kernel/interrupt/interrupt.h>
#include <kernel/mem/paging.h>
#include <kernel/mem/pmap.h>
#include "kmem.h"

/// Return flags for large-page leaf entries in level 3 (PDPT) and level 2
/// (PDT) tables.
static uint64_t
get_pdflags(uint32_t memtype)
{
    switch (memtype)
    {
        case PMEMTYPE_ACPI_NVS:
        case PMEMTYPE_UNCACHED:
            return PF_PRESENT | PF_GLOBAL | PF_SYSTEM |
                   PF_RW | PF_PS | PF_PWT | PF_PCD;

        case PMEMTYPE_BAD:
        case PMEMTYPE_UNMAPPED:
            return 0;

        case PMEMTYPE_USABLE:
        case PMEMTYPE_RESERVED:
        case PMEMTYPE_ACPI:
            return PF_PRESENT | PF_GLOBAL | PF_SYSTEM | PF_RW | PF_PS;

        default:
            fatal();
            return 0;
    }
}

/// Return flags for entries in level 1 (PT) tables.
static uint64_t
get_ptflags(uint32_t memtype)
{
    switch (memtype)
    {
        case PMEMTYPE_ACPI_NVS:
        case PMEMTYPE_UNCACHED:
            return PF_PRESENT | PF_GLOBAL | PF_SYSTEM |
                   PF_RW | PF_PWT | PF_PCD;

        case PMEMTYPE_BAD:
        case PMEMTYPE_UNMAPPED:
            return 0;

        case PMEMTYPE_USABLE:
        case PMEMTYPE_RESERVED:
        case PMEMTYPE_ACPI:
            return PF_PRESENT | PF_GLOBAL | PF_SYSTEM | PF_RW;

        default:
            fatal();
            return 0;
    }
}

/// Allocate the next available page in the kernel page table and return
/// its virtual address.
static inline uint64_t
alloc_page(pagetable_t *pt)
{
    if (pt->vnext >= pt->vterm)
        fatal();

    uint64_t vaddr = pt->vnext;
    pt->vnext += PAGE_SIZE;
    return vaddr | PF_SYSTEM | PF_PRESENT | PF_RW;
}

/// Create a 1GiB page entry in the kernel page table.
static void
create_huge_page(pagetable_t *pt, uint64_t addr, uint32_t memtype)
{
    uint64_t pml4te = PML4E(addr);
    uint64_t pdpte  = PDPTE(addr);

    page_t *pml4t = (page_t *)pt->proot;
    if (pml4t->entry[pml4te] == 0)
        pml4t->entry[pml4te] = alloc_page(pt);

    page_t *pdpt = PGPTR(pml4t->entry[pml4te]);
    pdpt->entry[pdpte] = addr | get_pdflags(memtype);
}

/// Create a 2MiB page entry in the kernel page table.
static void
create_large_page(pagetable_t *pt, uint64_t addr, uint32_t memtype)
{
    uint64_t pml4te = PML4E(addr);
    uint64_t pdpte  = PDPTE(addr);
    uint64_t pde    = PDE(addr);

    page_t *pml4t = (page_t *)pt->proot;
    if (pml4t->entry[pml4te] == 0)
        pml4t->entry[pml4te] = alloc_page(pt);

    page_t *pdpt = PGPTR(pml4t->entry[pml4te]);
    if (pdpt->entry[pdpte] == 0)
        pdpt->entry[pdpte] = alloc_page(pt);

    page_t *pdt = PGPTR(pdpt->entry[pdpte]);
    pdt->entry[pde] = addr | get_pdflags(memtype);
}

/// Create a 4KiB page entry in the kernel page table.
static void
create_small_page(pagetable_t *pt, uint64_t addr, uint32_t memtype)
{
    uint64_t pml4te = PML4E(addr);
    uint64_t pdpte  = PDPTE(addr);
    uint64_t pde    = PDE(addr);
    uint64_t pte    = PTE(addr);

    page_t *pml4t = (page_t *)pt->proot;
    if (pml4t->entry[pml4te] == 0)
        pml4t->entry[pml4te] = alloc_page(pt);

    page_t *pdpt = PGPTR(pml4t->entry[pml4te]);
    if (pdpt->entry[pdpte] == 0)
        pdpt->entry[pdpte] = alloc_page(pt);

    page_t *pdt = PGPTR(pdpt->entry[pdpte]);
    if (pdt->entry[pde] == 0)
        pdt->entry[pde] = alloc_page(pt);

    page_t *ptt = PGPTR(pdt->entry[pde]);
    ptt->entry[pte] = addr | get_ptflags(memtype);
}

/// Map a region of memory into the kernel page table, using the largest
/// page sizes possible.
static void
map_region(pagetable_t *pt, const pmap_t *map, const pmapregion_t *region)
{
    // Don't map bad (or unmapped) memory.
    if (region->type == PMEMTYPE_UNMAPPED || region->type == PMEMTYPE_BAD)
        return;

    // Don't map reserved regions beyond the last usable physical address.
    if (region->type == PMEMTYPE_RESERVED &&
        region->addr >= map->last_usable)
        return;

    uint64_t addr = region->addr;
    uint64_t term = region->addr + region->size;

    // Create a series of pages that cover the region. Try to use the largest
    // page sizes possible to keep the page table small.
    while (addr < term) {
        uint64_t remain = term - addr;

        // Create a huge page (1GiB) if possible.
        if ((addr & (PAGE_SIZE_HUGE - 1)) == 0 &&
            (remain >= PAGE_SIZE_HUGE)) {
            create_huge_page(pt, addr, region->type);
            addr += PAGE_SIZE_HUGE;
        }

        // Create a large page (2MiB) if possible.
        else if ((addr & (PAGE_SIZE_LARGE - 1)) == 0 &&
                 (remain >= PAGE_SIZE_LARGE)) {
            create_large_page(pt, addr, region->type);
            addr += PAGE_SIZE_LARGE;
        }

        // Create a small page (4KiB).
        else {
            create_small_page(pt, addr, region->type);
            addr += PAGE_SIZE;
        }
    }
}

void
kmem_init(pagetable_t *pt)
{
    // Zero all kernel page table memory.
    memzero((void *)KMEM_KERNEL_PAGETABLE, KMEM_KERNEL_PAGETABLE_SIZE);

    // Initialize the kernel page table.
    pt->proot = KMEM_KERNEL_PAGETABLE;
    pt->vroot = KMEM_KERNEL_PAGETABLE;
    pt->vnext = KMEM_KERNEL_PAGETABLE + PAGE_SIZE;
    pt->vterm = KMEM_KERNEL_PAGETABLE_END;

    // For each region in the physical memory map, create appropriate page
    // table entries.
    const pmap_t *map = pmap();
    for (uint64_t r = 0; r < map->count; r++)
        map_region(pt, map, &map->region[r]);
}
