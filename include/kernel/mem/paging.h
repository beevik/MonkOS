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

//----------------------------------------------------------------------------
//  @function   pagedb_init
/// @brief      Initialize the page frame database.
/// @details    The page frame database manages the physical memory used by
///             all memory pages known to the kernel.
//----------------------------------------------------------------------------
void
pagedb_init();

//----------------------------------------------------------------------------
//  @function   page_alloc
/// @brief      Allocate one or more pages contiguous in virtual memory.
/// @param[in]  count   The number of contiguous virtual memory pages to
///                     allocate.
/// @param[in]  flags   The PAGEFLAG_* flags assigned to each allocated page.
/// @return     A virtual memory pointer to the first page allocated.
//----------------------------------------------------------------------------
void *
page_alloc(int count, uint32_t flags);

// Page flags used with page_alloc.
#define PAGEFLAG_USER        (1 << 0)   ///< User level privileges.
#define PAGEFLAG_READONLY    (1 << 1)   ///< Read-only memory.
#define PAGEFLAG_EXEC        (1 << 2)   ///< Executable-only memory.
#define PAGEFLAG_NOSWAP      (1 << 3)   ///< Memory is never swapped to disk.

//----------------------------------------------------------------------------
//  @function   page_free
/// @brief      Free one or more contiguous pages from virtual memory.
/// @param[in]  addr    The virtual memory address of the first page to free.
/// @param[in]  count   The number of contiguous virtual memory pages to free.
//----------------------------------------------------------------------------
void
page_free(void *addr, int count);
