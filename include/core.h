//============================================================================
/// @file   core.h
/// @brief  Core include file.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

// Standard headers included by all code in the project.
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>

//----------------------------------------------------------------------------
// Macros
//----------------------------------------------------------------------------

/// Force a function to be inline, even in debug mode.
#define __forceinline        inline __attribute__((always_inline))

/// Return the number of elements in the C array.
#define arrsize(x)           ((int)(sizeof(x) / sizeof(x[0])))

/// Generic min/max routines
#define min(a, b)            ((a) < (b) ? (a) : (b))
#define max(a, b)            ((a) > (b) ? (a) : (b))

/// Compile-time static assertion
#define STATIC_ASSERT(a, b)  _Static_assert(a, b)
