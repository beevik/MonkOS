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
//  @function   strlcpy
/// @brief      Copy the source string to the destination buffer.
/// @details    Appends string src to the end of dst. It will copy at most
///             dstsize - 1 characters. A null terminator is added to the end
///             of the destination string unless dstsize was 0.
/// @param[in]  dst     Pointer to the destination buffer.
/// @param[in]  src     Pointer to the source string.
/// @param[in]  dstsize Maximum number of characters in the dst buffer after
///                     copying, including the null terminator.
/// @return     The length of the copied string after truncation.
//----------------------------------------------------------------------------
size_t
strlcpy(char *dst, const char *src, size_t dstsize);

//----------------------------------------------------------------------------
//  @function   strlcat
/// @brief      Append the source string to the end of the destination string.
/// @details    The function will append at most dstsize - strlen(dst) - 1
///             characters. A null terminator is added to the end of the
///             concatenated string unless dstsize was 0.
/// @param[in]  dst     Pointer to the destination string.
/// @param[in]  src     Pointer to the source string.
/// @param[in]  dstsize Maximum number of characters in the dst buffer after
///                     concatenation, including the null terminator.
/// @return     The length of the concatenated string after truncation.
//----------------------------------------------------------------------------
size_t
strlcat(char *dst, const char *src, size_t dstsize);

//----------------------------------------------------------------------------
//  @function   memcpy
/// @brief      Copy bytes from one memory region to another.
/// @details    If the memory regions overlap, this function's behavior
///             is undefined, and you should use memmove instead.
/// @param[in]  dst     Address of the destination memory area.
/// @param[in]  src     Address of the source memory area.
/// @param[in]  num     Number of bytes to copy.
/// @return             Destination address.
//----------------------------------------------------------------------------
void *
memcpy(void *dst, const void *src, size_t num);

//----------------------------------------------------------------------------
//  @function   memmove
/// @brief      Move bytes from one memory region to another, even if the
///             regions overlap.
/// @param[in]  dst     Address of the destination memory area.
/// @param[in]  src     Address of the source memory area.
/// @param[in]  num     Number of bytes to copy.
/// @return             Destination address.
//----------------------------------------------------------------------------
void *
memmove(void *dst, const void *src, size_t num);

//----------------------------------------------------------------------------
//  @function   memset
/// @brief      Fill a region of memory with a single byte value.
/// @param[in]  dst     Address of the destination memory area.
/// @param[in]  b       Value of the byte used to fill memory.
/// @param[in]  num     Number of bytes to set.
/// @return             Destination address.
//----------------------------------------------------------------------------
void *
memset(void *dst, int b, size_t num);

//----------------------------------------------------------------------------
//  @function   memsetw
/// @brief      Fill a region of memory with a single 16-bit word value.
/// @param[in]  dst     Address of the destination memory area.
/// @param[in]  w       Value of the word used to fill memory.
/// @param[in]  num     Number of words to set.
/// @return             Destination address.
//----------------------------------------------------------------------------
void *
memsetw(void *dst, int w, size_t num);

//----------------------------------------------------------------------------
//  @function   memsetd
/// @brief      Fill a region of memory with a single 32-bit dword value.
/// @param[in]  dst     Address of the destination memory area.
/// @param[in]  d       Value of the dword used to fill memory.
/// @param[in]  num     Number of dwords to set.
/// @return             Destination address.
//----------------------------------------------------------------------------
void *
memsetd(void *dst, uint32_t d, size_t num);

//----------------------------------------------------------------------------
//  @function   memzero
/// @brief      Fill a region of memory with zeroes.
/// @param[in]  dst     Address of the destination memory area.
/// @param[in]  num     Number of bytes to set to zero.
/// @return             Destination address.
//----------------------------------------------------------------------------
void *
memzero(void *dst, size_t num);
