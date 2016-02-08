//============================================================================
/// @file       stdio.h
/// @brief      Standard i/o library.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <core.h>
#include <stdarg.h>

//----------------------------------------------------------------------------
//  @function   snprintf
/// @brief      Compose a printf-formatted string into the target buffer.
/// @details    If the length of the string is greater than n-1 characters,
///             truncate the remaining characters but return the total number
///             of characters processed.
/// @param[in]  buf     Pointer to a buffer where the resulting string is
///                     to be stored.
/// @param[in]  n       Maximum number of characters in the the buffer,
///                     including space for a null terminator.
/// @param[in]  format  A format-specifier string.
/// @param[in]  ...     A variable argument list based on the contents of the
///                     format string.
/// @returns    The number of characters that would have been written if
///             n was sufficiently large.
//----------------------------------------------------------------------------
int
snprintf(char *buf, size_t n, const char *format, ...);

//----------------------------------------------------------------------------
//  @function   vsnprintf
/// @brief      Compose a printf-formatted string into the target buffer using
///             a variable argument list.
/// @details    If the length of the string is greater than n-1 characters,
///             truncate the remaining characters but return the total number
///             of characters processed.
/// @param[in]  buf     Pointer to a buffer where the resulting string is
///                     to be stored.
/// @param[in]  n       Maximum number of characters in the the buffer,
///                     including space for a null terminator.
/// @param[in]  format  A format-specifier string.
/// @param[in]  arg     Variable arguments list to be initialized with
///                     va_start.
/// @returns    The number of characters that would have been written if
///             n was sufficiently large.
//----------------------------------------------------------------------------
int
vsnprintf(char *buf, size_t n, const char *format, va_list arg);
