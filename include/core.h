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

#define __forceinline   inline __attribute__((always_inline))
