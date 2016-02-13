//============================================================================
/// @file       table.h
/// @brief      Physical memory table describing usable and reserved regions
///             of physical memory.
/// @details    Most of the table is derived from data provided by the system
///             BIOS at boot time.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <core.h>

//----------------------------------------------------------------------------
//  @enum       memtype
/// @brief      The types of memory reported in a memory table.
//----------------------------------------------------------------------------
enum memtype
{
    MEMTYPE_USABLE   = 1,
    MEMTYPE_RESERVED = 2,
    MEMTYPE_ACPI     = 3,
    MEMTYPE_ACPI_NVS = 4,
    MEMTYPE_BAD      = 5,
};

//----------------------------------------------------------------------------
//  @struct     memregion_t
/// @brief      A memregion represents and describes a contiguous region of
///             physical memory.
//----------------------------------------------------------------------------
struct memregion
{
    uint64_t addr;      ///< base address
    uint64_t size;      ///< size of memory region
    int32_t  type;      ///< Memory type (see memtype enum)
    uint32_t flags;     ///< flags (currently unused)
};
typedef struct memregion   memregion_t;

//----------------------------------------------------------------------------
//  @struct     memtable_t
/// @brief      A memory table describing available and reserved regions of
///             physical memory.
/// @details    There are no gaps in a memory table.
//----------------------------------------------------------------------------
struct memtable
{
    uint64_t    count;        ///< The number of memory regions in the table
    uint64_t    reserved;
    memregion_t region[1];    ///< An array of 'count' memory regions
};
typedef struct memtable   memtable_t;

//----------------------------------------------------------------------------
//  @function   memtable_init
/// @brief      Initialize the memory table using information provided by the
///             BIOS during boot loading.
//----------------------------------------------------------------------------
void
memtable_init();

//----------------------------------------------------------------------------
//  @function   memtable
/// @brief      Return a pointer to the current memory table.
/// @return     A pointer to the memory table.
//----------------------------------------------------------------------------
const memtable_t *
memtable();
