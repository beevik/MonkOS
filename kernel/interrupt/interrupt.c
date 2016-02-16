//============================================================================
/// @file       interrupt.c
/// @brief      Interrupt handling operations.
//
//  Copyright 2016 Brett Vickers.
//  Use of this source code is governed by a BSD-style license
//  that can be found in the MonkOS LICENSE file.
//============================================================================

#include <core.h>
#include <kernel/interrupt/interrupt.h>

void
fatal()
{
    RAISE_INTERRUPT(0xff);
}
