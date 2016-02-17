//============================================================================
/// @file       segments.h
/// @brief      Memory segment definitions.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <core.h>

// Segment selector values for segment registers.
#define SEGMENT_SELECTOR_KERNEL_DATA  0x08
#define SEGMENT_SELECTOR_KERNEL_CODE  0x10
#define SEGMENT_SELECTOR_USER_DATA    0x18
#define SEGMENT_SELECTOR_USER_CODE    0x20
#define SEGMENT_SELECTOR_TSS          0x28
