//============================================================================
/// @file   kernel/asm/interrupt.h
/// @brief  Inline assembly for interrupt-related behaviors.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <stdint.h>
#include <kernel/asm/asm.h>

//----------------------------------------------------------------------------
//  @function   interrupts_enable
/// @brief      Enable interrupts.
//----------------------------------------------------------------------------
force_inline void
interrupts_enable()
{
    asm volatile ("sti");
}

//----------------------------------------------------------------------------
//  @function   interrupts_disable
/// @brief      Disable interrupts.
//----------------------------------------------------------------------------
force_inline void
interrupts_disable()
{
    asm volatile ("cli");
}

//----------------------------------------------------------------------------
//  @function   halt
/// @brief      Halt the CPU until an interrupt occurs.
//----------------------------------------------------------------------------
force_inline void
halt()
{
    asm volatile ("hlt");
}
