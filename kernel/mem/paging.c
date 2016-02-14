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

struct state
{
    const memtable_t *memtable;  ///< Pointer to memory table.
    uint64_t          pfdbsize;  ///< Number of entries in pfdb.
    pf_t             *pfdb;      ///< Page frame database
    uint64_t          pfavail;   ///< Total available physical pages
};
typedef struct state state_t;

static state_t state;

void
pagedb_init()
{
    // Calculate total available memory
    state.memtable = memtable();
    for (unsigned i = 0; i < state.memtable->count; i++) {
        if (state.memtable->region[i].type == MEMTYPE_USABLE)
            state.pfavail += state.memtable->region[i].size / PAGE_SIZE;
    }

    // Find a contiguous region of memory large enough for the pagedb
    state.pfdbsize = state.memtable->region[state.memtable->count - 1].size /
                     PAGE_SIZE;

    // Build a kernel page table (rebuilding boot-loader-installed table)

    // Install page fault handler
}
