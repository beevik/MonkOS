//============================================================================
/// @file       timer.h
/// @brief      Programmable interval timer (8253/8254) controller.
//
//  Copyright 2016 Brett Vickers.
//  Use of this source code is governed by a BSD-style license
//  that can be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <core.h>

//----------------------------------------------------------------------------
//  @function   timer_init
/// @brief      Initialize the timer controller so that it interrupts the
///             kernel at the requested frequency.
///
/// @details    Interrupts are enabled at the end of the function, so
///             timer_enable does not need to be called after timer_init.
///
///             Due to the clock granularity (1193181Hz), the requested
///             frequency may not be perfectly met.
///
/// @param[in]  frequency   The interrupt frequency in Hz. Clamped to the
///                         range [19:1193181].
//----------------------------------------------------------------------------
void
timer_init(uint32_t frequency);

//----------------------------------------------------------------------------
//  @function   timer_enable
/// @brief      Enable timer interrupts.
//----------------------------------------------------------------------------
void
timer_enable();

//----------------------------------------------------------------------------
//  @function   timer_disable
/// @brief      Disable timer interrupts.
//----------------------------------------------------------------------------
void
timer_disable();
