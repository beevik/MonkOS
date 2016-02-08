//============================================================================
/// @file   string.h
/// @brief  String and memory operations.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <core.h>

//----------------------------------------------------------------------------
//  @function   strlen
/// @brief      Return the length of a null-terminated string.
/// @details    Count the number of characters in the null-terminated string
///             and return the count.
/// @param[in]  str     Pointer to a null-terminated string.
/// @return             The number of characters preceding the null
///                     terminator.
//----------------------------------------------------------------------------
size_t
strlen(const char *str);

//----------------------------------------------------------------------------
//  @function   memcpy
/// @brief      Copy bytes from one memory region to another.
/// @details    If the memory regions overlap, this function's behavior
///             is undefined, and you should use memmove instead.
/// @param[in]  dest    Address of the destination memory area.
/// @param[in]  src     Address of the source memory area.
/// @param[in]  num     Number of bytes to copy.
/// @return             Destination address.
//----------------------------------------------------------------------------
void *
memcpy(void *dest, const void *src, size_t num);

//----------------------------------------------------------------------------
//  @function   memmove
/// @brief      Move bytes from one memory region to another, even if the
///             regions overlap.
/// @param[in]  dest    Address of the destination memory area.
/// @param[in]  src     Address of the source memory area.
/// @param[in]  num     Number of bytes to copy.
/// @return             Destination address.
//----------------------------------------------------------------------------
void *
memmove(void *dest, const void *src, size_t num);

//----------------------------------------------------------------------------
//  @function   memset
/// @brief      Fill a region of memory with a single byte value.
/// @param[in]  dest    Address of the destination memory area.
/// @param[in]  b       Value of the byte used to fill memory.
/// @param[in]  num     Number of bytes to set.
/// @return             Destination address.
//----------------------------------------------------------------------------
void *
memset(void *dest, int b, size_t num);

//----------------------------------------------------------------------------
//  @function   memsetw
/// @brief      Fill a region of memory with a single 16-bit word value.
/// @param[in]  dest    Address of the destination memory area.
/// @param[in]  w       Value of the word used to fill memory.
/// @param[in]  num     Number of words to set.
/// @return             Destination address.
//----------------------------------------------------------------------------
void *
memsetw(void *dest, int w, size_t num);

//----------------------------------------------------------------------------
//  @function   memsetd
/// @brief      Fill a region of memory with a single 32-bit dword value.
/// @param[in]  dest    Address of the destination memory area.
/// @param[in]  d       Value of the dword used to fill memory.
/// @param[in]  num     Number of dwords to set.
/// @return             Destination address.
//----------------------------------------------------------------------------
void *
memsetd(void *dest, uint32_t d, size_t num);

//----------------------------------------------------------------------------
//  @function   memzero
/// @brief      Fill a region of memory with zeroes.
/// @param[in]  dest    Address of the destination memory area.
/// @param[in]  num     Number of bytes to set to zero.
/// @return             Destination address.
//----------------------------------------------------------------------------
void *
memzero(void *dest, size_t num);
