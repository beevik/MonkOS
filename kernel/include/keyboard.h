//============================================================================
/// @file       keyboard.h
/// @brief      Keyboard input routines.
//
//  Copyright 2016 Brett Vickers.
//  Use of this source code is governed by a BSD-style license
//  that can be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <stdint.h>

// Meta-key bit masks
#define META_SHIFT      (1 << 0)
#define META_CTRL       (1 << 1)
#define META_ALT        (1 << 2)

// Toggle-key bit masks
#define TOGGLE_CAPSLOCK (1 << 0)
#define TOGGLE_NUMLOCK  (1 << 1)
#define TOGGLE_SCRLOCK  (1 << 2)

//----------------------------------------------------------------------------
//  @enum       keycode_t
/// @brief      Key code values used to initialize a keyboard scan map.
//----------------------------------------------------------------------------
typedef enum keycode
{
    KEY_CTRL        = 0x81,
    KEY_SHIFT       = 0x82,
    KEY_ALT         = 0x83,
    KEY_PRTSCR      = 0x90,
    KEY_CAPSLOCK    = 0x91,
    KEY_NUMLOCK     = 0x92,
    KEY_SCRLOCK     = 0x93,
    KEY_INSERT      = 0xa0,
    KEY_END         = 0xa1,
    KEY_DOWN        = 0xa2,
    KEY_PGDN        = 0xa3,
    KEY_LEFT        = 0xa4,
    KEY_CENTER      = 0xa5,     ///< Keypad Center
    KEY_RIGHT       = 0xa6,
    KEY_HOME        = 0xa7,
    KEY_UP          = 0xa8,
    KEY_PGUP        = 0xa9,
    KEY_DEL         = 0xaa,
    KEY_MINUS       = 0xab,     ///< Keypad Minus
    KEY_PLUS        = 0xac,     ///< Keypad Plus
    KEY_F1          = 0xb0,
    KEY_F2          = 0xb1,
    KEY_F3          = 0xb2,
    KEY_F4          = 0xb3,
    KEY_F5          = 0xb4,
    KEY_F6          = 0xb5,
    KEY_F7          = 0xb6,
    KEY_F8          = 0xb7,
    KEY_F9          = 0xb8,
    KEY_F10         = 0xb9,
    KEY_F11         = 0xba,
    KEY_F12         = 0xbb,
    KEY_INVALID     = 0xff,
} keycode_t;

//----------------------------------------------------------------------------
//  @struct     scanmap
/// @brief      A map of keyboard scan codes to characters values.
//----------------------------------------------------------------------------
typedef struct
scanmap
{
    uint8_t shifted[128];       // scancode -> keycode when shift key down
    uint8_t unshifted[128];     // scancode -> keycode when shift key up
} scanmap_t;

//----------------------------------------------------------------------------
//  @function   kb_init
/// @brief      Initialize the keyboard so that it can provide input to the
///             kernel.
/// @details    kb_init installs the default US English PS/2 scan map.
//----------------------------------------------------------------------------
void
kb_init();

//----------------------------------------------------------------------------
//  @function   kb_getchar
/// @brief      Return the next character from the keyboard's input buffer.
/// @details    If the buffer is empty, return 0.
/// @returns    The ascii value of the next character in the input buffer,
///             or 0 if there are no characters available.
//----------------------------------------------------------------------------
char
kb_getchar();

//----------------------------------------------------------------------------
//  @function   kb_lastscancode
/// @brief      Return the last scan code received by the keyboard.
/// @returns    Last scan code received by the keyboard.
//----------------------------------------------------------------------------
uint8_t
kb_lastscancode();

//----------------------------------------------------------------------------
//  @function   kb_meta
/// @brief      Return the meta-key bit mask.
/// @returns    The meta-key bitmask.
//----------------------------------------------------------------------------
uint8_t
kb_meta();

//----------------------------------------------------------------------------
//  @function   kb_toggle
/// @brief      Return the toggle-key bit mask.
/// @returns    The toggle-key bitmask.
//----------------------------------------------------------------------------
uint8_t
kb_toggle();

//----------------------------------------------------------------------------
//  @function   kb_installscanmap
/// @brief      Install a new keyboard scan map.
/// @param[in]  map     The scan map to install.
//----------------------------------------------------------------------------
void
kb_installscanmap(scanmap_t *map);
