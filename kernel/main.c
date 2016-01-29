//============================================================================
/// @file       main.c
/// @brief      The kernel's main entry point.
/// @details    This file contains the function kmain(), which is the first
///             function called by the kernel's start code in start.asm.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <interrupt.h>
#include <io.h>

#if defined(__linux__)
#   error "This code must be compiled with a cross-compiler."
#endif

static int key_irqs;

//----------------------------------------------------------------------------
//  @function   handle_irq_keyboard
/// @brief      Interrupt service routine for keyboard input interrupt.
/// @param[in]  interrupt   Interrupt number.
/// @param[in]  error       Interrupt error code.
//----------------------------------------------------------------------------
static void
handle_irq_keyboard(uint8_t interrupt, uint64_t error)
{
    (void)interrupt;
    (void)error;

    key_irqs++;

    // Read the key value.
    uint8_t value = io_inb(0x60);
    (void)value;

    // Send end-of-interrupt signal.
    io_outb(0x20, 0x20);
}


//----------------------------------------------------------------------------
//  @function   kmain
/// @brief      Kernel main entry point.
/// @details    kmain is the first function called by the kernel bootstrapper
///             in start.asm.
//----------------------------------------------------------------------------
void
kmain()
{
    // Return here, causing the system to hang. The rest of the kernel
    // will go here.

    // Initialize all interrupt data structures.
    interrupts_init();

    // Assign interrupt service routine handlers.
    isr_set(0x21, handle_irq_keyboard);

    // Enable hardware interrupts we wish to handle.
    irq_enable(1);  // IRQ1 = keyboard

    // Turn on interrupt service routines.
    interrupts_enable();

    for (;;) {
        halt();
    }
}
