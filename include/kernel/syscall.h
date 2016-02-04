//============================================================================
/// @file       syscall.h
/// @brief      System call support.
//
//  Copyright 2016 Brett Vickers.
//  Use of this source code is governed by a BSD-style license
//  that can be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <stdint.h>

//----------------------------------------------------------------------------
//  @function   syscall_init
/// @brief      Set up the CPU to handle system calls.
//----------------------------------------------------------------------------
void
syscall_init();
