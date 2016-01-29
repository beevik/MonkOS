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
#include <console.h>

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
    // Initialize the console screen.
    console_init();
    console_set_textcolor(0, TEXTCOLOR_WHITE, TEXTCOLOR_BLACK);
    console_clear(0);

    // Initialize all interrupt data structures.
    interrupts_init();

    // Assign interrupt service routine handlers.
    isr_set(0x21, handle_irq_keyboard);

    // Enable hardware interrupts we wish to handle.
    irq_enable(1);  // IRQ1 = keyboard

    // Turn on interrupt service routines.
    interrupts_enable();

    // Display a welcome message on each virtual console.
    for (int id = 0; id < MAX_CONSOLES; id++) {
        console_print(id, "Welcome to \033[e]MonkOS\033[-] (v0.1).\n");
        console_set_textcolor_fg(id, TEXTCOLOR_LTGRAY);
    }

    for (;;) {
        halt();

        // Output some text to test console scrolling.
        if (key_irqs % 2 == 0) {
            char buf[] = "Test \033[e] \033[-]\n";
            char ch = 'a' + (key_irqs / 2) % 26;
            buf[9] = ch;
            console_print(0, buf);
        }
    }
}
