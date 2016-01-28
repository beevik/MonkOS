//============================================================================
/// @file strings.h
///
/// String and memory operations.
///
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

/*
 *  Memory operations
 *  -----------------
 */

/// Copy bytes from one memory region to another. If the memory regions
/// overlap, this function's behavior is undefined, and you should use
/// memmove instead.
/// @param[in]  dest    Address of the destination memory area.
/// @param[in]  src     Address of the source memory area.
/// @param[in]  num     Number of bytes to copy.
/// @return             Destination address.
void *
memcpy(void *dest, const void *src, size_t num);

/// Move bytes from one memory region to another, even if the regions
/// overlap.
/// @param[in]  dest    Address of the destination memory area.
/// @param[in]  src     Address of the source memory area.
/// @param[in]  num     Number of bytes to copy.
/// @return             Destination address.
void *
memmove(void *dest, const void *src, size_t num);

/// Fill a region of memory with a single byte value.
/// @param[in]  dest    Address of the destination memory area.
/// @param[in]  ch      Value of the byte used to fill memory.
/// @param[in]  num     Number of bytes to fill.
/// @return             Destination address.
void *
memset(void *dest, int ch, size_t num);

/// Fill a region of memory with zeroes.
/// @param[in]  dest    Address of the destination memory area.
/// @param[in]  num     Number of bytes to set to zero.
/// @return             Destination address.
void *
memzero(void *dest, size_t num);
