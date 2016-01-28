//============================================================================
/// @file interrupt.h
///
/// Interrupt handling operations.
///
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

/// Initialize the interrupt service routine table. Clear all interrupt
/// handlers and set up the CPU to properly handle all exceptions.
void
isr_init();

/// Handler function called when an interrupt exception occurs.
/// @param[in]  interrupt   Interrupt number (0-255).
/// @param[in]  error       Error code (interrupt-specific meaning).
typedef void (* isr_handler)(uint8_t interrupt, uint64_t error);

/// Set an interrupt service routine for the given interrupt number.
/// @param[in]  interrupt   Interrupt number (0-255).
/// @param[in]  handler     Interrupt service routine handler function.
void
isr_set(int interrupt, isr_handler handler);
