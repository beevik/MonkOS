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
#include <kernel/mem/kmem.h>
#include <kernel/mem/paging.h>
#include <kernel/mem/pmap.h>

static uint64_t kroot;  /// The kernel page table's root physical address.

/// Kernel page table data structure, used while building the kernel's
/// page table.
typedef struct kpagetable
{
    uint64_t root;   // Physical address of the root PML4T table
    uint64_t next;   // Next available address for a table page
    uint64_t term;   // Boundary for table page addresses
} kpagetable_t;

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

/// Allocate the next available page in the kernel page table.
static inline uint64_t
alloc_page(kpagetable_t *kpt)
{
    if (kpt->next >= kpt->term)
        fatal();

    uint64_t addr = kpt->next;
    kpt->next += PAGE_SIZE;
    return addr | PF_SYSTEM | PF_PRESENT | PF_RW;
}

/// Create a 1GiB page entry in the kernel page table.
static void
create_huge_page(kpagetable_t *kpt, uint64_t addr, uint32_t memtype)
{
    uint64_t pml4te = PML4E(addr);
    uint64_t pdpte  = PDPTE(addr);

    page_t *pml4t = (page_t *)kpt->root;
    if (pml4t->entry[pml4te] == 0)
        pml4t->entry[pml4te] = alloc_page(kpt);

    page_t *pdpt = PGPTR(pml4t->entry[pml4te]);
    pdpt->entry[pdpte] = addr | get_pdflags(memtype);
}

/// Create a 2MiB page entry in the kernel page table.
static void
create_large_page(kpagetable_t *kpt, uint64_t addr, uint32_t memtype)
{
    uint64_t pml4te = PML4E(addr);
    uint64_t pdpte  = PDPTE(addr);
    uint64_t pde    = PDE(addr);

    page_t *pml4t = (page_t *)kpt->root;
    if (pml4t->entry[pml4te] == 0)
        pml4t->entry[pml4te] = alloc_page(kpt);

    page_t *pdpt = PGPTR(pml4t->entry[pml4te]);
    if (pdpt->entry[pdpte] == 0)
        pdpt->entry[pdpte] = alloc_page(kpt);

    page_t *pdt = PGPTR(pdpt->entry[pdpte]);
    pdt->entry[pde] = addr | get_pdflags(memtype);
}

/// Create a 4KiB page entry in the kernel page table.
static void
create_small_page(kpagetable_t *kpt, uint64_t addr, uint32_t memtype)
{
    uint64_t pml4te = PML4E(addr);
    uint64_t pdpte  = PDPTE(addr);
    uint64_t pde    = PDE(addr);
    uint64_t pte    = PTE(addr);

    page_t *pml4t = (page_t *)kpt->root;
    if (pml4t->entry[pml4te] == 0)
        pml4t->entry[pml4te] = alloc_page(kpt);

    page_t *pdpt = PGPTR(pml4t->entry[pml4te]);
    if (pdpt->entry[pdpte] == 0)
        pdpt->entry[pdpte] = alloc_page(kpt);

    page_t *pdt = PGPTR(pdpt->entry[pdpte]);
    if (pdt->entry[pde] == 0)
        pdt->entry[pde] = alloc_page(kpt);

    page_t *pt = PGPTR(pdt->entry[pde]);
    pt->entry[pte] = addr | get_ptflags(memtype);
}

/// Map a region of memory into the kernel page table, using the largest
/// page sizes possible.
static void
map_region(kpagetable_t *kpt, const pmap_t *table, const pmapregion_t *region)
{
    // Don't map bad (or unmapped) memory.
    if (region->type == PMEMTYPE_UNMAPPED || region->type == PMEMTYPE_BAD)
        return;

    // Don't map reserved regions beyond the last usable physical address.
    if (region->type == PMEMTYPE_RESERVED &&
        region->addr >= table->last_usable)
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
            create_huge_page(kpt, addr, region->type);
            addr += PAGE_SIZE_HUGE;
        }

        // Create a large page (2MiB) if possible.
        else if ((addr & (PAGE_SIZE_LARGE - 1)) == 0 &&
                 (remain >= PAGE_SIZE_LARGE)) {
            create_large_page(kpt, addr, region->type);
            addr += PAGE_SIZE_LARGE;
        }

        // Create a small page (4KiB).
        else {
            create_small_page(kpt, addr, region->type);
            addr += PAGE_SIZE;
        }
    }
}

uint64_t
kmem_init()
{
    // Zero all kernel page table memory.
    memzero((void *)KMEM_KERNEL_PAGETABLE, KMEM_KERNEL_PAGETABLE_SIZE);

    // Initialize the kernel page table.
    kpagetable_t kpt;
    kpt.root = KMEM_KERNEL_PAGETABLE;
    kpt.next = KMEM_KERNEL_PAGETABLE + PAGE_SIZE;
    kpt.term = KMEM_KERNEL_PAGETABLE_END;

    // For each region in the physical memory map, create appropriate page
    // table entries.
    const pmap_t *table = pmap();
    for (uint64_t r = 0; r < table->count; r++)
        map_region(&kpt, table, &table->region[r]);

    // Tell the CPU to start using the kernel page table.
    kroot = kpt.root;
    return kroot;
}

uint64_t
kmem_pagetable_addr()
{
    return kroot;
}
