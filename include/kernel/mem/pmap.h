//============================================================================
/// @file       pmap.h
/// @brief      Physical memory map describing usable and reserved regions
///             of physical memory.
/// @details    Most of the map is derived from data provided by the system
///             BIOS at boot time.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <core.h>

//----------------------------------------------------------------------------
//  @enum       pmemtype
/// @brief      The types of physical memory.
//----------------------------------------------------------------------------
enum pmemtype
{
    PMEMTYPE_USABLE   = 1,   ///< Reported usable by the BIOS.
    PMEMTYPE_RESERVED = 2,   ///< Reported (or inferred) to be reserved.
    PMEMTYPE_ACPI     = 3,   ///< Used for ACPI tables or code.
    PMEMTYPE_ACPI_NVS = 4,   ///< Used for ACPI non-volatile storage.
    PMEMTYPE_BAD      = 5,   ///< Reported as bad memory.
    PMEMTYPE_UNCACHED = 6,   ///< Marked as uncacheable, usually for I/O.
    PMEMTYPE_UNMAPPED = 7,   ///< Marked as "do not map".
};

//----------------------------------------------------------------------------
//  @struct     pmapregion_t
/// @brief      A memregion represents and describes a contiguous region of
///             physical memory.
//----------------------------------------------------------------------------
struct pmapregion
{
    uint64_t addr;               ///< base address
    uint64_t size;               ///< size of memory region
    int32_t  type;               ///< Memory type (see memtype enum)
    uint32_t flags;              ///< flags (currently unused)
};
typedef struct pmapregion pmapregion_t;

//----------------------------------------------------------------------------
//  @struct     pmap_t
/// @brief      A memory map describing available and reserved regions of
///             physical memory.
/// @details    There are no gaps in a memory map.
//----------------------------------------------------------------------------
struct pmap
{
    uint64_t     count;          ///< Memory regions in the memory map
    uint64_t     last_usable;    ///< End of last usable region
    pmapregion_t region[1];      ///< An array of 'count' memory regions
};
typedef struct pmap pmap_t;

//----------------------------------------------------------------------------
//  @function   pmap_init
/// @brief      Initialize the physical memory map using data installed by the
///             BIOS during boot loading.
//----------------------------------------------------------------------------
void
pmap_init();

//----------------------------------------------------------------------------
//  @function   pmap_add
/// @brief      Add a region of memory to the physical memory map.
/// @param[in]  addr    The starting address of the region.
/// @param[in]  size    The size of the region.
/// @param[in]  type    The type of memory to add.
//----------------------------------------------------------------------------
void
pmap_add(uint64_t addr, uint64_t size, enum pmemtype type);

//----------------------------------------------------------------------------
//  @function   pmap
/// @brief      Return a pointer to the current physical memory map.
/// @returns    A pointer to the physical memory map.
//----------------------------------------------------------------------------
const pmap_t *
pmap();
