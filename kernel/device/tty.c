//============================================================================
/// @file       tty.c
/// @brief      Teletype (console) screen text manipulation routines.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#include <core.h>
#include <libc/stdio.h>
#include <libc/string.h>
#include <kernel/x86/cpu.h>
#include <kernel/device/tty.h>

// CRTC ports
#define CRTC_PORT_CMD           0x03d4   ///< Command port for CRT controller.
#define CRTC_PORT_DATA          0x03d5   ///< Data port for CRT controller.

// CRTC commands
#define CRTC_CMD_STARTADDR_HI   0x0c   ///< Hi-byte of buffer start address.
#define CRTC_CMD_STARTADDR_LO   0x0d   ///< Lo-byte of buffer start address.
#define CRTC_CMD_CURSORADDR_HI  0x0e   ///< Hi-byte of cursor start address.
#define CRTC_CMD_CURSORADDR_LO  0x0f   ///< Lo-byte of cursor start address.

// Visible screen geometry
#define SCREEN_ROWS             25
#define SCREEN_COLS             80
#define SCREEN_SIZE             (SCREEN_ROWS * SCREEN_COLS)
#define SCREEN_BUFFER           0x000b8000

/// Virtual console state.
struct tty
{
    uint16_t    textcolor;       ///< Current fg/bg color (shifted).
    uint16_t    textcolor_orig;  ///< Original, non-override text color.
    screenpos_t pos;             ///< Current screen position.
    uint8_t     ybuf;            ///< Virtual buffer y position.
    uint16_t   *screen;          ///< Virtual screen buffer for 50 rows.
    uint16_t   *tlcorner;        ///< Points to char in top-left corner.
};

typedef struct tty tty_t;

static tty_t  tty[MAX_TTYS];     ///< All virtual consoles.
static tty_t *active_tty;        ///< The currently visible console.

static inline uint16_t
color(textcolor_t fg, textcolor_t bg)
{
    return (uint16_t)bg << 12 | (uint16_t)fg << 8;
}

static void
update_buffer_offset()
{
    // Calculate top-left corner offset from the start of the first screen
    // buffer.
    int offset = (int)(active_tty->tlcorner - (uint16_t *)SCREEN_BUFFER);

    uint8_t save = io_inb(CRTC_PORT_CMD);

    io_outb(CRTC_PORT_CMD, CRTC_CMD_STARTADDR_LO);
    io_outb(CRTC_PORT_DATA, (uint8_t)offset);
    io_outb(CRTC_PORT_CMD, CRTC_CMD_STARTADDR_HI);
    io_outb(CRTC_PORT_DATA, (uint8_t)(offset >> 8));

    io_outb(CRTC_PORT_CMD, save);
}

static void
update_cursor()
{
    // Calculate cursor offset from the start of the first screen buffer.
    int offset = active_tty->ybuf * SCREEN_COLS + active_tty->pos.x +
                 (int)(active_tty->screen - (uint16_t *)SCREEN_BUFFER);

    uint8_t save = io_inb(CRTC_PORT_CMD);

    io_outb(CRTC_PORT_CMD, CRTC_CMD_CURSORADDR_LO);
    io_outb(CRTC_PORT_DATA, (uint8_t)offset);
    io_outb(CRTC_PORT_CMD, CRTC_CMD_CURSORADDR_HI);
    io_outb(CRTC_PORT_DATA, (uint8_t)(offset >> 8));

    io_outb(CRTC_PORT_CMD, save);
}

void
tty_init()
{
    uint16_t *screenptr = (uint16_t *)SCREEN_BUFFER;

    for (int id = 0; id < MAX_TTYS; id++) {
        tty[id].textcolor      = color(TEXTCOLOR_WHITE, TEXTCOLOR_BLACK);
        tty[id].textcolor_orig = tty[id].textcolor;
        tty[id].pos.x          = 0;
        tty[id].pos.y          = 0;
        tty[id].ybuf           = 0;
        tty[id].screen         = screenptr;
        tty[id].tlcorner       = screenptr;
        screenptr             += 0x1000; // each screen is 4K words.
    }
    active_tty = &tty[0];
}

void
tty_activate(int id)
{
    if ((id < 0) || (id >= MAX_TTYS)) {
        id = 0;
    }
    if (&tty[id] == active_tty) {
        return;
    }

    active_tty = &tty[id];
    update_buffer_offset();
    update_cursor();
}

void
tty_set_textcolor(int id, textcolor_t fg, textcolor_t bg)
{
    if ((id < 0) || (id >= MAX_TTYS)) {
        id = 0;
    }

    tty[id].textcolor = tty[id].textcolor_orig = color(fg, bg);
}

void
tty_set_textcolor_fg(int id, textcolor_t fg)
{
    if ((id < 0) || (id >= MAX_TTYS)) {
        id = 0;
    }

    tty[id].textcolor      = color(fg, tty_get_textcolor_bg(id));
    tty[id].textcolor_orig = tty[id].textcolor;
}

void
tty_set_textcolor_bg(int id, textcolor_t bg)
{
    if ((id < 0) || (id >= MAX_TTYS)) {
        id = 0;
    }

    tty[id].textcolor      = color(tty_get_textcolor_fg(id), bg);
    tty[id].textcolor_orig = tty[id].textcolor;
}

textcolor_t
tty_get_textcolor_fg(int id)
{
    if ((id < 0) || (id >= MAX_TTYS)) {
        id = 0;
    }

    return (textcolor_t)((tty[id].textcolor_orig >> 8) & 0x0f);
}

textcolor_t
tty_get_textcolor_bg(int id)
{
    if ((id < 0) || (id >= MAX_TTYS)) {
        id = 0;
    }

    return (textcolor_t)((tty[id].textcolor_orig >> 12) & 0x0f);
}

void
tty_clear(int id)
{
    if ((id < 0) || (id >= MAX_TTYS)) {
        id = 0;
    }

    memsetw(tty[id].screen,
            tty[id].textcolor | ' ', SCREEN_SIZE * 2);
    tty[id].pos.x    = 0;
    tty[id].pos.y    = 0;
    tty[id].ybuf     = 0;
    tty[id].tlcorner = tty[id].screen;

    if (active_tty == &tty[id]) {
        update_buffer_offset();
        update_cursor();
    }
}

void
tty_setpos(int id, screenpos_t pos)
{
    if ((id < 0) || (id >= MAX_TTYS)) {
        id = 0;
    }

    int diff = (int)pos.y - (int)tty[id].pos.y;
    tty[id].pos  = pos;
    tty[id].ybuf = (uint8_t)((int)tty[id].ybuf + diff);
    if (active_tty == &tty[id]) {
        update_cursor();
    }
}

void
tty_getpos(int id, screenpos_t *pos)
{
    if ((id < 0) || (id >= MAX_TTYS)) {
        id = 0;
    }
    *pos = tty[id].pos;
}

static int
colorcode(char x, int orig)
{
    int code = x;

    if ((code >= '0') && (code <= '9')) {
        return code - '0';
    }
    else if ((code >= 'a') && (code <= 'f')) {
        return code - 'a' + 10;
    }
    else if ((code >= 'A') && (code <= 'F')) {
        return code - 'A' + 10;
    }
    else if (code == '-') {
        return orig;
    }
    else {
        return -1;
    }
}

static void
tty_printchar(tty_t *cons, const char **strptr)
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
    else if (ch == '\b') {
        if (cons->pos.x > 0) {
            int offset = cons->ybuf * SCREEN_COLS + --cons->pos.x;
            cons->screen[offset] = cons->textcolor | ' ';
        }
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
            if (cons->ybuf == SCREEN_ROWS * 2) {
                cons->ybuf -= SCREEN_ROWS;
            }

            // Clear the row at the bottom of the screen.
            memsetw(cons->screen + cons->ybuf * SCREEN_COLS,
                    cons->textcolor | ' ',
                    SCREEN_COLS * sizeof(uint16_t));

            // Adjust the offset of the top-left corner of the screen.
            cons->tlcorner = cons->screen + (cons->ybuf + 1) * SCREEN_COLS -
                             SCREEN_SIZE;

            // Do a hardware scroll if this console is currently active.
            if (cons == active_tty) {
                update_buffer_offset();
            }
        }
    }
}

void
tty_print(int id, const char *str)
{
    if ((id < 0) || (id >= MAX_TTYS))
        id = 0;

    tty_t *cons = &tty[id];
    for (; *str; ++str)
        tty_printchar(cons, &str);

    if (cons == active_tty)
        update_cursor();
}

void
tty_printc(int id, char ch)
{
    if ((id < 0) || (id >= MAX_TTYS))
        id = 0;

    const char  str[2] = { ch, 0 };
    const char *ptr    = str;
    tty_t      *cons   = &tty[id];
    tty_printchar(cons, &ptr);

    if (cons == active_tty)
        update_cursor();
}

int
tty_printf(int id, const char *format, ...)
{
    if ((id < 0) || (id >= MAX_TTYS))
        id = 0;

    va_list args;
    va_start(args, format);
    char buffer[8 * 1024];
    int  result = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    tty_print(id, buffer);

    return result;
}
