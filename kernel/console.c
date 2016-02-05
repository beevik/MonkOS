//============================================================================
/// @file       console.c
/// @brief      Console screen text manipulation routines.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#include <core.h>
#include <libc/string.h>
#include <kernel/console.h>
#include <kernel/io.h>

// CRTC ports
#define CRTC_PORT_CMD     0x03d4         ///< Command port for CRT controller.
#define CRTC_PORT_DATA    0x03d5         ///< Data port for CRT controller.

// CRTC commands
#define CRTC_CMD_STARTADDR_HI     0x0c  ///< Hi-byte of buffer start address.
#define CRTC_CMD_STARTADDR_LO     0x0d  ///< Lo-byte of buffer start address.
#define CRTC_CMD_CURSORADDR_HI    0x0e  ///< Hi-byte of cursor start address.
#define CRTC_CMD_CURSORADDR_LO    0x0f  ///< Lo-byte of cursor start address.

// Visible screen geometry
#define SCREEN_ROWS      25
#define SCREEN_COLS      80
#define SCREEN_SIZE      (SCREEN_ROWS * SCREEN_COLS)
#define SCREEN_BUFFER    0x000b8000

//----------------------------------------------------------------------------
//  @struct     console_t
//  @brief      The full state of a virtual console.
//----------------------------------------------------------------------------
typedef struct console
{
    uint16_t    textcolor;          ///< Current fg/bg color (shifted left 8).
    uint16_t    textcolor_orig;     ///< Original, non-override text color.
    screenpos_t pos;                ///< Current screen position.
    uint8_t     ybuf;               ///< Virtual buffer y position.
    uint16_t   *screen;             ///< Virtual screen buffer for 50 rows.
    uint16_t   *tlcorner;           ///< Points to char in top-left corner.
} console_t;

static console_t  console[MAX_CONSOLES];   ///< All virtual consoles.
static console_t *active_console;          ///< The currently visible console.

//----------------------------------------------------------------------------
//  @function   color
//  @brief      Return a 16-bit textcolor field that can be quickly or'd with
//              a character before being placed into the screen buffer.
//----------------------------------------------------------------------------
static inline uint16_t
color(textcolor_t fg, textcolor_t bg)
{
    return (uint16_t)bg << 12 | (uint16_t)fg << 8;
}

//----------------------------------------------------------------------------
//  @function   update_buffer_offset
//  @brief      Adjust the video hardware's buffer offset based on the
//              active console's state.
//----------------------------------------------------------------------------
static void
update_buffer_offset()
{
    // Calculate top-left corner offset from the start of the first screen
    // buffer.
    int offset = (int)(active_console->tlcorner - (uint16_t *)SCREEN_BUFFER);

    uint8_t save = io_inb(CRTC_PORT_CMD);

    io_outb(CRTC_PORT_CMD, CRTC_CMD_STARTADDR_LO);
    io_outb(CRTC_PORT_DATA, (uint8_t)offset);
    io_outb(CRTC_PORT_CMD, CRTC_CMD_STARTADDR_HI);
    io_outb(CRTC_PORT_DATA, (uint8_t)(offset >> 8));

    io_outb(CRTC_PORT_CMD, save);
}

//----------------------------------------------------------------------------
//  @function   update_cursor
//  @brief      Adjust the video hardware's cursor position based on the
//              active console's state.
//----------------------------------------------------------------------------
static void
update_cursor()
{
    // Calculate cursor offset from the start of the first screen buffer.
    int offset = active_console->ybuf * SCREEN_COLS + active_console->pos.x +
                 (int)(active_console->screen - (uint16_t *)SCREEN_BUFFER);

    uint8_t save = io_inb(CRTC_PORT_CMD);

    io_outb(CRTC_PORT_CMD, CRTC_CMD_CURSORADDR_LO);
    io_outb(CRTC_PORT_DATA, (uint8_t)offset);
    io_outb(CRTC_PORT_CMD, CRTC_CMD_CURSORADDR_HI);
    io_outb(CRTC_PORT_DATA, (uint8_t)(offset >> 8));

    io_outb(CRTC_PORT_CMD, save);
}

//----------------------------------------------------------------------------
//  @function   console_init
//  @brief      Initialize all virtual consoles.
//  @details    This function must be called before any other console
//              functions can be used.
//----------------------------------------------------------------------------
void
console_init()
{
    uint16_t *screenptr = (uint16_t *)SCREEN_BUFFER;

    for (int id = 0; id < MAX_CONSOLES; id++) {
        console[id].textcolor      = color(TEXTCOLOR_WHITE, TEXTCOLOR_BLACK);
        console[id].textcolor_orig = console[id].textcolor;
        console[id].pos.x          = 0;
        console[id].pos.y          = 0;
        console[id].ybuf           = 0;
        console[id].screen         = screenptr;
        console[id].tlcorner       = screenptr;
        screenptr += 0x1000;    // each screen is 4K words (8K bytes).
    }
    active_console = &console[0];
}

//----------------------------------------------------------------------------
//  @function   console_activate
//  @brief      Activate the requested virtual console.
//  @details    The virtual console's buffer is immediately displayed on the
//              screen.
//  @param[in]  id      Virtual console id (0-3).
//----------------------------------------------------------------------------
void
console_activate(int id)
{
    if ((id < 0) || (id >= MAX_CONSOLES))
        id = 0;
    if (&console[id] == active_console)
        return;

    active_console = &console[id];
    update_buffer_offset();
    update_cursor();
}

//----------------------------------------------------------------------------
//  @function   console_set_textcolor
//  @brief      Set the foreground and background colors used to display
//              text on the virtual console.
//  @param[in]  id      Virtual console id (0-3).
//  @param[in]  fg      Foreground color.
//  @param[in]  bg      Background color.
//----------------------------------------------------------------------------
void
console_set_textcolor(int id, textcolor_t fg, textcolor_t bg)
{
    if ((id < 0) || (id >= MAX_CONSOLES))
        id = 0;

    console[id].textcolor = console[id].textcolor_orig = color(fg, bg);
}

//----------------------------------------------------------------------------
//  @function   console_set_textcolor_fg
//  @brief      Set the foreground color used to display text on the virtual
//              console.
//  @param[in]  id      Virtual console id (0-3).
//  @param[in]  fg      Foreground color.
//----------------------------------------------------------------------------
void
console_set_textcolor_fg(int id, textcolor_t fg)
{
    if ((id < 0) || (id >= MAX_CONSOLES))
        id = 0;

    console[id].textcolor      = color(fg, console_get_textcolor_bg(id));
    console[id].textcolor_orig = console[id].textcolor;
}

//----------------------------------------------------------------------------
//  @function   console_set_textcolor_bg
//  @brief      Set the background color used to display text on the virtual
//              console.
//  @param[in]  id      Virtual console id (0-3).
//  @param[in]  bg      Background color.
//----------------------------------------------------------------------------
void
console_set_textcolor_bg(int id, textcolor_t bg)
{
    if ((id < 0) || (id >= MAX_CONSOLES))
        id = 0;

    console[id].textcolor      = color(console_get_textcolor_fg(id), bg);
    console[id].textcolor_orig = console[id].textcolor;
}

//----------------------------------------------------------------------------
//  @function   console_get_textcolor_fg
//  @brief      Get the foreground color used to display text on the virtual
//              console.
//  @param[in]  id      Virtual console id (0-3).
//  @returns    Foreground color.
//----------------------------------------------------------------------------
textcolor_t
console_get_textcolor_fg(int id)
{
    if ((id < 0) || (id >= MAX_CONSOLES))
        id = 0;

    return (textcolor_t)((console[id].textcolor_orig >> 8) & 0x0f);
}

//----------------------------------------------------------------------------
//  @function   console_get_textcolor_bg
//  @brief      Get the background color used to display text on the virtual
//              console.
//  @param[in]  id      Virtual console id (0-3).
//  @returns    Background color.
//----------------------------------------------------------------------------
textcolor_t
console_get_textcolor_bg(int id)
{
    if ((id < 0) || (id >= MAX_CONSOLES))
        id = 0;

    return (textcolor_t)((console[id].textcolor_orig >> 12) & 0x0f);
}

//----------------------------------------------------------------------------
//  @function   console_clear
//  @brief      Clear the virtual console screen's contents using the current
//              text background color.
//  @param[in]  id      Virtual console id (0-3).
//----------------------------------------------------------------------------
void
console_clear(int id)
{
    if ((id < 0) || (id >= MAX_CONSOLES))
        id = 0;

    memsetw(console[id].screen,
            console[id].textcolor | ' ', SCREEN_SIZE * 2);
    console[id].pos.x    = 0;
    console[id].pos.y    = 0;
    console[id].ybuf     = 0;
    console[id].tlcorner = console[id].screen;
}

//----------------------------------------------------------------------------
//  @function   console_setpos
//  @brief      Set the position of the cursor on the virtual console.
//  @details    Text written to the console after this function will be
//              located at the requested screen position.
//  @param[in]  id      Virtual console id (0-3).
//  @param[in]  pos     The screen position of the cursor.
//----------------------------------------------------------------------------
void
console_setpos(int id, screenpos_t pos)
{
    if ((id < 0) || (id >= MAX_CONSOLES))
        id = 0;

    int diff = (int)pos.y - (int)console[id].pos.y;
    console[id].pos  = pos;
    console[id].ybuf = (uint8_t)((int)console[id].ybuf + diff);
    if (active_console == &console[id])
        update_cursor();
}

//----------------------------------------------------------------------------
//  @function   console_getpos
//  @brief      Get the current position of the cursor on the virtual console.
//  @param[in]  id      Virtual console id (0-3).
//  @param[out] pos     A pointer to a screenpos_t to receive the position.
//----------------------------------------------------------------------------
void
console_getpos(int id, screenpos_t *pos)
{
    if ((id < 0) || (id >= MAX_CONSOLES))
        id = 0;
    *pos = console[id].pos;
}

//----------------------------------------------------------------------------
//  @function   colorcode
/// @brief      Return the textcolor corresponding to the escape sequence
///             character 'x' contained in \033[x].
/// @param[in]  x       The escape sequence character.
/// @param[in]  orig    The console's original color. Used when x is '-'.
/// @returns    Return the textcolor as an integer in the range [0:15] if
///             x is valid. Otherwise return -1.
//----------------------------------------------------------------------------
static int
colorcode(char x, int orig)
{
    int code = x;

    if ((code >= '0') && (code <= '9'))
        return code - '0';
    else if ((code >= 'a') && (code <= 'f'))
        return code - 'a' + 10;
    else if ((code >= 'A') && (code <= 'F'))
        return code - 'A' + 10;
    else if (code == '-')
        return orig;
    else
        return -1;
}

//----------------------------------------------------------------------------
//  @function   console_printchar
/// @brief      Print a single character to the virtual console.
/// @details    Use the console's current text color and screen position. A
///             newline character ('\\n') causes the screen position to
///             be updated as though a carriage return and line feed were
///             performed.
/// @param[in]      cons    Virtual console pointer.
/// @param[in,out]  strptr  Pointer to the string.
//----------------------------------------------------------------------------
static void
console_printchar(console_t *cons, const char **strptr)
{
    bool linefeed = false;

    const char *str = *strptr;
    char        ch  = *str;

    // If the newline character is encountered, do a line feed + carriage
    // return.
    if (ch == '\n') {
        cons->pos.x = 0;
        linefeed    = true;
    }
    // Handle color codes, e.g. "\033[#]".
    else if (ch == '\033') {
        if ((str[1] == '[') && str[2] && (str[3] == ']')) {
            int code = colorcode(str[2], (cons->textcolor_orig >> 8) & 0x0f);
            if (code != -1) {
                cons->textcolor = (cons->textcolor & 0xf000) |
                                  (uint16_t)(code << 8);
                *strptr += 3;
            }
        }
        else if ((str[1] == '{') && str[2] && (str[3] == '}')) {
            int code = colorcode(str[2], (cons->textcolor_orig >> 12));
            if (code != -1) {
                cons->textcolor = (cons->textcolor & 0x0f00) |
                                  (uint16_t)(code << 12);
                *strptr += 3;
            }
        }
        return;
    }
    else {
        // Use the current foreground and background color.
        uint16_t value = cons->textcolor | ch;

        // Calculate the buffer offset using the virtual row.
        int offset = cons->ybuf * SCREEN_COLS + cons->pos.x;

        // Update the visible screen buffer.
        cons->screen[offset] = value;

        // If the right side of the screen was reached, we need a linefeed.
        if (++cons->pos.x == SCREEN_COLS) {
            cons->pos.x = 0;
            linefeed    = true;
        }
    }

    // A linefeed causes a hardware scroll of one row. If we reach the end of
    // the virtual buffer, wrap it back one screen.
    if (linefeed) {

        // Copy the last line of the screen to a virtual row one screen
        // away. This way, when we reach the end of the virtual buffer,
        // we'll have another shifted copy of the screen ready to display.
        // This is better than copying the entire screen whenever we reach
        // the end of the virtual buffer, because it amortizes the cost
        // of copying.
        memcpy(cons->screen + cons->ybuf * SCREEN_COLS - SCREEN_SIZE,
               cons->screen + cons->ybuf * SCREEN_COLS,
               SCREEN_COLS * sizeof(uint16_t));

        // Increment row (on screen and in the virtual buffer).
        ++cons->pos.y;
        ++cons->ybuf;

        // If we're at the bottom of the screen, we need to scroll a line.
        if (cons->pos.y == SCREEN_ROWS) {

            --cons->pos.y;

            // If we're at the end of the virtual buffer, we need to
            // wrap back a screen.
            if (cons->ybuf == SCREEN_ROWS * 2)
                cons->ybuf -= SCREEN_ROWS;

            // Clear the row at the bottom of the screen.
            memsetw(cons->screen + cons->ybuf * SCREEN_COLS,
                    cons->textcolor | ' ',
                    SCREEN_COLS * sizeof(uint16_t));

            // Adjust the offset of the top-left corner of the screen.
            cons->tlcorner = cons->screen + (cons->ybuf + 1) * SCREEN_COLS -
                             SCREEN_SIZE;

            // Do a hardware scroll if this console is currently active.
            if (cons == active_console)
                update_buffer_offset();
        }
    }
}

//----------------------------------------------------------------------------
//  @function   console_print
//  @brief      Output a null-terminated string to the virtual console using
//              the console's current text color and screen position.
//  @param[in]  id      Virtual console id (0-3).
//  @param[in]  str     The null-terminated string to be printed.
//----------------------------------------------------------------------------
void
console_print(int id, const char *str)
{
    if ((id < 0) || (id >= MAX_CONSOLES))
        id = 0;

    console_t *cons = &console[id];
    for (; *str; ++str) {
        console_printchar(cons, &str);
    }

    if (cons == active_console)
        update_cursor();
}
