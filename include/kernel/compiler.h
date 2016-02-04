//============================================================================
/// @file   compiler.h
/// @brief  Compiler-directed keywords
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#define force_inline    inline __attribute__((always_inline))
