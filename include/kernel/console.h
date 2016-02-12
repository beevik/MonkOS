//============================================================================
/// @file       console.h
/// @brief      Console screen text manipulation routines.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <core.h>
#include <stdarg.h>

//----------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------

/// The number of available virtual consoles.
#define MAX_CONSOLES    4

//----------------------------------------------------------------------------
//  @enum       textcolor_t
/// @brief      Color values used for console text.
//----------------------------------------------------------------------------
enum textcolor
{
    TEXTCOLOR_BLACK     = 0,
    TEXTCOLOR_BLUE      = 1,
    TEXTCOLOR_GREEN     = 2,
    TEXTCOLOR_CYAN      = 3,
    TEXTCOLOR_RED       = 4,
    TEXTCOLOR_MAGENTA   = 5,
    TEXTCOLOR_BROWN     = 6,
    TEXTCOLOR_LTGRAY    = 7,
    TEXTCOLOR_GRAY      = 8,
    TEXTCOLOR_LTBLUE    = 9,
    TEXTCOLOR_LTGREEN   = 10,
    TEXTCOLOR_LTCYAN    = 11,
    TEXTCOLOR_LTRED     = 12,
    TEXTCOLOR_LTMAGENTA = 13,
    TEXTCOLOR_YELLOW    = 14,
    TEXTCOLOR_WHITE     = 15,
};

typedef enum textcolor   textcolor_t;

//----------------------------------------------------------------------------
//  @struct     screenpos_t
/// @brief      Console screen text position.
//----------------------------------------------------------------------------
struct screenpos
{
    uint8_t x;      ///< x position in range [0:79]. 0 is left of screen.
    uint8_t y;      ///< y position in range [0:24]. 0 is top of screen.
};

typedef struct screenpos   screenpos_t;

//----------------------------------------------------------------------------
//  @function   console_init
/// @brief      Initialize all virtual consoles.
/// @details    This function must be called before any other console
///             functions can be used.
//----------------------------------------------------------------------------
void
console_init();

//----------------------------------------------------------------------------
//  @function   console_activate
/// @brief      Activate the requested virtual console.
/// @details    The virtual console's buffer is immediately displayed on the
///             screen.
/// @param[in]  id      Virtual console id (0-3).
//----------------------------------------------------------------------------
void
console_activate(int id);

//----------------------------------------------------------------------------
//  @function   console_set_textcolor
/// @brief      Set the foreground and background colors used to display
///             text on the virtual console.
/// @param[in]  id      Virtual console id (0-3).
/// @param[in]  fg      Foreground color.
/// @param[in]  bg      Background color.
//----------------------------------------------------------------------------
void
console_set_textcolor(int id, textcolor_t fg, textcolor_t bg);

//----------------------------------------------------------------------------
//  @function   console_set_textcolor_fg
/// @brief      Set the foreground color used to display text on the virtual
///             console.
/// @param[in]  id      Virtual console id (0-3).
/// @param[in]  fg      Foreground color.
//----------------------------------------------------------------------------
void
console_set_textcolor_fg(int id, textcolor_t fg);

//----------------------------------------------------------------------------
//  @function   console_set_textcolor_bg
/// @brief      Set the background color used to display text on the virtual
///             console.
/// @param[in]  id      Virtual console id (0-3).
/// @param[in]  bg      Background color.
//----------------------------------------------------------------------------
void
console_set_textcolor_bg(int id, textcolor_t bg);

//----------------------------------------------------------------------------
//  @function   console_get_textcolor_fg
/// @brief      Get the foreground color used to display text on the virtual
///             console.
/// @param[in]  id      Virtual console id (0-3).
/// @returns    Foreground color.
//----------------------------------------------------------------------------
textcolor_t
console_get_textcolor_fg(int id);

//----------------------------------------------------------------------------
//  @function   console_get_textcolor_bg
/// @brief      Get the background color used to display text on the virtual
///             console.
/// @param[in]  id      Virtual console id (0-3).
/// @returns    Background color.
//----------------------------------------------------------------------------
textcolor_t
console_get_textcolor_bg(int id);

//----------------------------------------------------------------------------
//  @function   console_clear
/// @brief      Clear the virtual console screen's contents using the current
///             text background color.
/// @param[in]  id      Virtual console id (0-3).
//----------------------------------------------------------------------------
void
console_clear(int id);

//----------------------------------------------------------------------------
//  @function   console_setpos
/// @brief      Set the position of the cursor on the virtual console.
/// @details    Text written to the console after this function will be
///             located at the requested screen position.
/// @param[in]  id      Virtual console id (0-3).
/// @param[in]  pos     The screen position of the cursor.
//----------------------------------------------------------------------------
void
console_setpos(int id, screenpos_t pos);

//----------------------------------------------------------------------------
//  @function   console_getpos
/// @brief      Get the current position of the cursor on the virtual console.
/// @param[in]  id      Virtual console id (0-3).
/// @param[out] pos     A pointer to a screenpos_t to receive the position.
//----------------------------------------------------------------------------
void
console_getpos(int id, screenpos_t *pos);

//----------------------------------------------------------------------------
//  @function   console_print
/// @brief      Output a null-terminated string to the virtual console using
///             the console's current text color and screen position.
///
/// @details    A newline character (\\n) causes the screen position to
///             be updated as though a carriage return and line feed were
///             performed.
///
///             To change the foreground color on the fly without having to
///             call a console function, you may use the escape sequence
///             \033[x], where x is a single character representing the
///             foreground color to use for all following text. If x is a
///             hexadecimal digit, then it represents one of the 16 textcolor
///             codes. If x is '-', then it represents the console's
///             original foreground color setting.
///
///             To change the background color on the fly, use the escape
///             sequence \033{x}. The meaning of x is the same as with
///             the foreground color escape sequence.
/// @param[in]  id      Virtual console id (0-3).
/// @param[in]  str     The null-terminated string to be printed.
//----------------------------------------------------------------------------
void
console_print(int id, const char *str);

//----------------------------------------------------------------------------
//  @function   console_printf
/// @brief      Output a printf-formatted string to the virtual console using
///             the console's current text color and screen position.
/// @details    See console_print for further details.
/// @param[in]  id      Virtual console id (0-3).
/// @param[in]  format  The null-terminated format string used to format the
///                     text to be printed.
/// @param[in]  ...     Variable arguments list to be initialized with
///                     va_start.
/// @return     The number of characters written to the console.
//----------------------------------------------------------------------------
int
console_printf(int id, const char *format, ...);
