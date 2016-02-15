//============================================================================
/// @file       paging.h
/// @brief      Virtual memory paging.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <core.h>

// Paging constants
#define PAGE_SIZE    4096

typedef uint64_t pagetable_handle;

//----------------------------------------------------------------------------
//  @function   pagedb_init
/// @brief      Initialize the page frame database.
/// @details    The page frame database manages the physical memory used by
///             all memory pages known to the kernel.
//----------------------------------------------------------------------------
void
pagedb_init();

//----------------------------------------------------------------------------
//  @function   pagetable_create
/// @brief      Create a new page table that can be used to allocate virtual
///             memory.
/// @return     A handle to the created page table.
//----------------------------------------------------------------------------
pagetable_handle
pagetable_create();

//----------------------------------------------------------------------------
//  @function   pagetable_destroy
/// @brief      Destroy a page table.
/// @param[in]  A handle to the page table to destroy.
//----------------------------------------------------------------------------
void
pagetable_destroy(pagetable_handle pt);

//----------------------------------------------------------------------------
//  @function   pagetable_activate
/// @brief      Activate a page table on the CPU, so all virtual memory
///             operations are performed relative to the page table.
/// @param[in]  A handle to the activated page table.
//----------------------------------------------------------------------------
void
pagetable_activate(pagetable_handle pt);

//----------------------------------------------------------------------------
//  @function   page_alloc
/// @brief      Allocate one or more pages contiguous in virtual memory.
/// @param[in]  pt      Handle to the page table from which to allocate the
///                     page(s).
/// @param[in]  count   The number of contiguous virtual memory pages to
///                     allocate.
/// @param[in]  flags   The PAGEFLAG_* flags assigned to each allocated page.
/// @return     A virtual memory pointer to the first page allocated.
//----------------------------------------------------------------------------
void *
page_alloc(pagetable_handle pt, int count, uint32_t flags);

//----------------------------------------------------------------------------
//  @function   page_free
/// @brief      Free one or more contiguous pages from virtual memory.
/// @param[in]  pt      Handle to ehte page table from which to free the
///                     page(s).
/// @param[in]  addr    The virtual memory address of the first page to free.
/// @param[in]  count   The number of contiguous virtual memory pages to free.
//----------------------------------------------------------------------------
void
page_free(pagetable_handle pt, void *addr, int count);
