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
#include <keyboard.h>
#include <console.h>

#if defined(__linux__)
#   error "This code must be compiled with a cross-compiler."
#endif

//----------------------------------------------------------------------------
//  @function   hexchar
/// @brief      Convert a 4-byte integer to its hexadecimal representation.
/// @param[in]  value   An integer value between 0 and 15.
/// @returns    A character representing the hexadecimal digit.
//----------------------------------------------------------------------------
static inline char
hexchar(int value)
{
    if (value >= 0 && value <= 9)
        return (char)(value + '0');
    else
        return (char)(value - 10 + 'a');
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

    // Initialize the keyboard.
    kb_init();

    // Turn on interrupt service routines.
    interrupts_enable();

    // Display a welcome message on each virtual console.
    for (int id = 0; id < MAX_CONSOLES; id++) {
        console_print(id, "Welcome to \033[e]MonkOS\033[-] (v0.1).\n");
        console_set_textcolor_fg(id, TEXTCOLOR_LTGRAY);
    }

    for (;;) {
        halt();

        // Output the last keyboard scan code every time there's an interrupt.
        for (char ch = kb_getchar(); ch; ch = kb_getchar()) {
            uint8_t scancode = kb_lastscancode();
            char buf[] = "Scan code: \033[e]  \033[-] ' '\n";
            buf[15] = hexchar(scancode >> 4);
            buf[16] = hexchar(scancode & 0xf);
            buf[23] = ch;
            console_print(0, buf);
        }
    }
}
