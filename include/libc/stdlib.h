//============================================================================
/// @file       stdlib.h
/// @brief      General utilities library.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <core.h>

//----------------------------------------------------------------------------
//  @typedef    sortcmp
/// @brief      Comparison function pointer type for use with the qsort
///             function.
/// @param[in]  a       Pointer to first element to be compared.
/// @param[in]  b       Pointer to second element to be compared.
/// @return     <0 if the element pointed to by a should go before the
///             element pointed to by b.
///
///             0 if the elements sort equivalently.
///
///             >0 if the element pointed to by a should go after the
///             element pointed to by b.
//----------------------------------------------------------------------------
typedef int (*sortcmp)(const void *a, const void *b);

//----------------------------------------------------------------------------
//  @function   qsort
/// @brief      Sort the elements of an array using a sort comparison
///             function.
/// @details    Sorts a contiguous array of \b num elements pointed to by
///             \b base, where each element is \b size bytes long.
/// @param[in]  base    Pointer to the first element in the array.
/// @param[in]  num     Number of element in the array to be sorted.
/// @param[in]  size    Size of each element in bytes.
/// @param[in]  cmp     Pointer to a function that compares two elements.
//----------------------------------------------------------------------------
void
qsort(void *base, size_t num, size_t size, sortcmp cmp);
