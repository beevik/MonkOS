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
#include <kernel/mem/table.h>
#include <kernel/mem/paging.h>

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

uint64_t pfdbs;

void
pagedb_init()
{
    // Calculate total available memory
    const memtable_t *table = memtable();
    uint64_t          mem   = 0;
    for (unsigned i = 0; i < table->count; i++) {
        if (table->region[i].type == MEMTYPE_USABLE)
            mem += table->region[i].size;
    }

    // Find a contiguous region of memory large enough for the pagedb
    uint64_t pagesNeeded = (mem / PAGE_SIZE);
    uint64_t pfdb_size   = pagesNeeded * sizeof(pf_t);
    pfdbs = pfdb_size;

    // Build a kernel page table (rebuilding boot-loader-installed table)

    // Install page fault handler
}
