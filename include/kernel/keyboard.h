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
#define META_SHIFT       (1 << 0)   ///< Set while the shift key is pressed.
#define META_CTRL        (1 << 1)   ///< Set while the ctrl key is pressed.
#define META_ALT         (1 << 2)   ///< Set while the alt key is pressed.
#define META_ESCAPED     (1 << 3)   ///< Set if key's scan code is escaped.
#define META_CAPSLOCK    (1 << 4)   ///< Set while caps lock is on.
#define META_NUMLOCK     (1 << 5)   ///< Set while num lock is on.
#define META_SCRLOCK     (1 << 6)   ///< Set while scroll lock is on.

//----------------------------------------------------------------------------
//  @enum       keycode_t
/// @brief      Key code values representing individual keys on the keyboard.
/// @details    Key codes corresponding to printable characters are not
///             listed here, but they are equal in value to their lowercase
///             ASCII representations (e.g., KEY_A = 'a', KEY_1 = '1', etc.).
//----------------------------------------------------------------------------
typedef enum keycode
{
    KEY_BACKSPACE = 0x08,
    KEY_TAB       = 0x09,
    KEY_ENTER     = 0x0d,
    KEY_ESCAPE    = 0x1b,
    KEY_CTRL      = 0x81,
    KEY_SHIFT     = 0x82,
    KEY_ALT       = 0x83,
    KEY_PRTSCR    = 0x90,
    KEY_CAPSLOCK  = 0x91,
    KEY_NUMLOCK   = 0x92,
    KEY_SCRLOCK   = 0x93,
    KEY_INSERT    = 0xa0,
    KEY_END       = 0xa1,
    KEY_DOWN      = 0xa2,
    KEY_PGDN      = 0xa3,
    KEY_LEFT      = 0xa4,
    KEY_CENTER    = 0xa5,       ///< Keypad Center
    KEY_RIGHT     = 0xa6,
    KEY_HOME      = 0xa7,
    KEY_UP        = 0xa8,
    KEY_PGUP      = 0xa9,
    KEY_DEL       = 0xaa,
    KEY_MINUS     = 0xab,       ///< Keypad Minus
    KEY_PLUS      = 0xac,       ///< Keypad Plus
    KEY_F1        = 0xb0,
    KEY_F2        = 0xb1,
    KEY_F3        = 0xb2,
    KEY_F4        = 0xb3,
    KEY_F5        = 0xb4,
    KEY_F6        = 0xb5,
    KEY_F7        = 0xb6,
    KEY_F8        = 0xb7,
    KEY_F9        = 0xb8,
    KEY_F10       = 0xb9,
    KEY_F11       = 0xba,
    KEY_F12       = 0xbb,
    KEY_SCANESC   = 0xfe,       ///< Escaped scan code
    KEY_INVALID   = 0xff,       ///< Invalid scan code
} keycode_t;

//----------------------------------------------------------------------------
//  @struct     key_t
/// @brief      A record representing the state of the keyboard at the time
///             a key was presssed or unpressed.
//----------------------------------------------------------------------------
typedef struct key
{
    uint8_t brk;            ///< Break code. 0 = key down, 1 = key up.
    uint8_t meta;           ///< Metakey mask when key was generated.
    uint8_t code;           ///< Keycode value (type = keycode_t).
    uint8_t ch;             ///< Character value, if valid.
} key_t;

//----------------------------------------------------------------------------
//  @struct     keylayout_t
/// @brief      A map of keyboard scan codes to key codes.
//----------------------------------------------------------------------------
typedef struct keylayout
{
    uint8_t shifted[128];       ///< when shift key is down.
    uint8_t unshifted[128];     ///< when shift key is up.
} keylayout_t;

//----------------------------------------------------------------------------
//  @function   kb_init
/// @brief      Initialize the keyboard so that it can provide input to the
///             kernel.
/// @details    kb_init installs the default US English PS/2 keyboard layout.
//----------------------------------------------------------------------------
void
kb_init();

//----------------------------------------------------------------------------
//  @function   kb_setlayout
/// @brief      Install a new keyboard layout.
/// @param[in]  layout  The keyboard layout to install.
//----------------------------------------------------------------------------
void
kb_setlayout(keylayout_t *layout);

//----------------------------------------------------------------------------
//  @function   kb_getchar
/// @brief      Return the next available character from the keyboard's
///             input buffer.
/// @returns    The ascii value of the next character in the input buffer,
///             or 0 if there are no characters available.
//----------------------------------------------------------------------------
char
kb_getchar();

//----------------------------------------------------------------------------
//  @function   kb_getkey
/// @brief      Return the available next key from the keyboard's input
///             buffer.
/// @param[out] key     The key record of the next key in the buffer.
/// @returns    true if there is a key in the buffer, false otherwise.
//----------------------------------------------------------------------------
bool
kb_getkey(key_t *key);

//----------------------------------------------------------------------------
//  @function   kb_meta
/// @brief      Return the current meta-key bit mask.
/// @returns    The meta-key bitmask.
//----------------------------------------------------------------------------
uint8_t
kb_meta();
