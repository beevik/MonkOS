//============================================================================
/// @file       exception.c
/// @brief      CPU exceptions.
//
//  Copyright 2016 Brett Vickers.
//  Use of this source code is governed by a BSD-style license
//  that can be found in the MonkOS LICENSE file.
//============================================================================

#include <stdbool.h>
#include <stdint.h>
#include <kernel/exception.h>
#include <kernel/interrupt.h>
#include <kernel/console.h>

//----------------------------------------------------------------------------
//  @function   isr_abort
/// @brief      Handle an exception that requires the system to abort.
/// @param[in]  context     The CPU context at the time of the interrupt.
//----------------------------------------------------------------------------
static void
isr_abort(const interrupt_context_t *context)
{
    (void)context;

    console_activate(0);
    console_set_textcolor(0, TEXTCOLOR_WHITE, TEXTCOLOR_RED);
    console_clear(0);
    console_print(0, "Unrecoverable error");
    for (;;) {
        halt();
    }
}

//----------------------------------------------------------------------------
//  @function   isr_breakpoint
/// @brief      Handle a breakpoint exception.
/// @param[in]  context     The CPU context at the time of the interrupt.
//----------------------------------------------------------------------------
static void
isr_breakpoint(const interrupt_context_t *context)
{
    (void)context;

    console_print(0, "Breakpoint hit.\n");
}

//----------------------------------------------------------------------------
//  @function   exceptions_init
//  @brief      Initialize all exception handling routines.
//----------------------------------------------------------------------------
void
exceptions_init()
{
    isr_set(EXCEPTION_DOUBLE_FAULT, isr_abort);
    isr_set(EXCEPTION_COPROCESSOR, isr_abort);
    isr_set(EXCEPTION_MACHINE_CHECK, isr_abort);

    isr_set(EXCEPTION_BREAKPOINT, isr_breakpoint);
}
