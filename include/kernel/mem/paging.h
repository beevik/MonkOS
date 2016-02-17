//============================================================================
/// @file       paging.h
/// @brief      Paged memory management.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <core.h>

// Pag size constants
#define PAGE_SIZE          0x1000
#define PAGE_SIZE_LARGE    0x200000
#define PAGE_SIZE_HUGE     0x40000000

// Page table entry flags
#define PF_PRESENT         (1 << 0)
#define PF_RW              (1 << 1)
#define PF_USER            (1 << 2)
#define PF_PWT             (1 << 3) // Page write-thru
#define PF_PCD             (1 << 4) // Cache disable
#define PF_ACCESS          (1 << 5)
#define PF_DIRTY           (1 << 6)
#define PF_PS              (1 << 7) // Page size (valid for PD and PDPT only)
#define PF_GLOBAL          (1 << 8)

// Virtual address bitmasks and shifts
#define PGSHIFT_PML4E      39
#define PGSHIFT_PDPTE      30
#define PGSHIFT_PDE        21
#define PGSHIFT_PTE        12
#define PGMASK_ENTRY       0x1ff
#define PGMASK_OFFSET      0x3ff

// Virtual address subfield accessors
#define PML4E(a)      (((a) >> PGSHIFT_PML4E) & PGMASK_ENTRY)
#define PDPTE(a)      (((a) >> PGSHIFT_PDPTE) & PGMASK_ENTRY)
#define PDE(a)        (((a) >> PGSHIFT_PDE) & PGMASK_ENTRY)
#define PTE(a)        (((a) >> PGSHIFT_PTE) & PGMASK_ENTRY)

// Page table entry helpers
#define PGPTR(pte)    ((page_t *)((pte) & ~PGMASK_OFFSET))

//----------------------------------------------------------------------------
//  @union      page_t
/// @brief      A pagetable page record.
/// @details    Contains 512 page table entries if the page holds a page
///             table. Otherwise it contains 4096 bytes of memory.
//----------------------------------------------------------------------------
typedef union page
{
    uint64_t entry[PAGE_SIZE / sizeof(uint64_t)];
    uint8_t  memory[PAGE_SIZE];
} page_t;

//----------------------------------------------------------------------------
//  @struct     pagetable_t
/// @brief      A pagetable structure.
/// @details    Holds all the page table entries that map virtual addresses to
///             physical addresses.
//----------------------------------------------------------------------------
typedef struct pagetable
{
    page_t *pml4t;      ///< Page map level 4 table (root table)
    page_t *next;       ///< Next virtual page to use when growing the table
} pagetable_t;

//----------------------------------------------------------------------------
//  @function   page_init
/// @brief      Initialize the page frame database.
/// @details    The page frame database manages the physical memory used by
///             all memory pages known to the kernel.
//----------------------------------------------------------------------------
void
page_init();

//----------------------------------------------------------------------------
//  @function   pagetable_create
/// @brief      Create a new page table that can be used to associate virtual
///             addresses with physical addresses.
/// @param[in]  pt      A pointer to the pagetable structure that will hold
///                     the page table.
/// @param[in]  vaddr   The virtual address within the new page table where
///                     the page table will be mapped.
/// @return     A handle to the created page table.
//----------------------------------------------------------------------------
void
pagetable_create(pagetable_t *pt, void *vaddr);

//----------------------------------------------------------------------------
//  @function   pagetable_destroy
/// @brief      Destroy a page table.
/// @param[in]  pt      A handle to the page table to destroy.
//----------------------------------------------------------------------------
void
pagetable_destroy(pagetable_t *pt);

//----------------------------------------------------------------------------
//  @function   pagetable_activate
/// @brief      Activate a page table on the CPU, so all virtual memory
///             operations are performed relative to the page table.
/// @param[in]  pt      A handle to the activated page table.
//----------------------------------------------------------------------------
void
pagetable_activate(pagetable_t *pt);

//----------------------------------------------------------------------------
//  @function   page_alloc
/// @brief      Allocate one or more pages contiguous in virtual memory.
/// @param[in]  pt      Handle to the page table from which to allocate the
///                     page(s).
/// @param[in]  vaddr   The virtual address of the first allocated page.
/// @param[in]  count   The number of contiguous virtual memory pages to
///                     allocate.
/// @return     A virtual memory pointer to the first page allocated.
//----------------------------------------------------------------------------
void *
page_alloc(pagetable_t *pt, void *vaddr, int count);

//----------------------------------------------------------------------------
//  @function   page_free
/// @brief      Free one or more contiguous pages from virtual memory.
/// @param[in]  pt      Handle to ehte page table from which to free the
///                     page(s).
/// @param[in]  vaddr   The virtual address of the first allocated page.
/// @param[in]  count   The number of contiguous virtual memory pages to free.
//----------------------------------------------------------------------------
void
page_free(pagetable_t *pt, void *vaddr, int count);
