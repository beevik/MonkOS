//============================================================================
/// @file main.c
///
/// The kernel's main entry point.
///
/// This file contains the function kmain(), which is the first function
/// called by the kernel's start code in start.asm.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <strings.h>
#include <interrupt.h>

#if defined(__linux__)
#   error "This code must be compiled with a cross-compiler."
#endif

/// Kernel main entry point. This is the first function called by the
/// kernel bootstrapper in start.asm.
void
kmain()
{
    isr_init();

    // Return here, causing the system to hang. The rest of the kernel
    // will go here.
}
