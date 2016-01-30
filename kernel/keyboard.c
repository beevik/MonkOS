//============================================================================
/// @file       keyboard.c
/// @brief      Keyboard input routines.
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
#include <strings.h>
#include <keyboard.h>

// Keyboard I/O ports
#define KB_PORT_DATA    0x60    ///< Keyboard I/O data port.

// Other constants
#define BUFSIZ          128     ///< Keyboard input buffer size.

// Key code abbreviations, used to set up the default scan map table below.
#define CTL     KEY_CTRL
#define SH      KEY_SHIFT
#define ALT     KEY_ALT
#define PS      KEY_PRTSCR
#define CL      KEY_CAPSLOCK
#define NL      KEY_NUMLOCK
#define SL      KEY_SCRLOCK
#define KI      KEY_INSERT
#define KE      KEY_END
#define KD      KEY_DOWN
#define KPD     KEY_PGDN
#define KL      KEY_LEFT
#define KC      KEY_CENTER
#define KR      KEY_RIGHT
#define KH      KEY_HOME
#define KU      KEY_UP
#define KPU     KEY_PGUP
#define KDL     KEY_DEL
#define KM      KEY_MINUS
#define KP      KEY_PLUS
#define F1      KEY_F1
#define F2      KEY_F2
#define F3      KEY_F3
#define F4      KEY_F4
#define F5      KEY_F5
#define F6      KEY_F6
#define F7      KEY_F7
#define F8      KEY_F8
#define F9      KEY_F9
#define F10     KEY_F10
#define F11     KEY_F11
#define F12     KEY_F12
#define INV     KEY_INVALID

/// US English PS/2 keyboard scan map (default setting)
static const scanmap_t ps2 =
{
    .shifted =
    {
      //   0     1     2     3     4     5     6     7
      //   8     9     a     b     c     d     e     f
         INV,   27,  '!',  '@',  '#',  '$',  '%',  '^',
         '&',  '*',  '(',  ')',  '_',  '+',    8,    9,   // 0
         'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',
         'O',  'P',  '{',  '}',   13,  CTL,  'A',  'S',   // 1
         'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',
         '"',  '~',   SH,  '|',  'Z',  'X',  'C',  'V',   // 2
         'B',  'N',  'M',  '<',  '>',  '?',   SH,   PS,
         ALT,  ' ',   CL,   F1,   F2,   F3,   F4,   F5,   // 3
          F6,   F7,   F8,   F9,  F10,   NL,   SL,   KH,
          KU,  KPU,   KM,   KL,   KC,   KR,   KP,   KE,   // 4
          KD,  KPD,   KI,  KDL,  INV,  INV,  INV,  F11,
         F12,  INV,  INV,  INV,  INV,  INV,  INV,  INV,   // 5
         INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV,
         INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV,   // 6
         INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV,
         INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV,   // 7
    },
    .unshifted =
    {
      //   0     1     2     3     4     5     6     7
      //   8     9     a     b     c     d     e     f
         INV,   27,  '1',  '2',  '3',  '4',  '5',  '6',
         '7',  '8',  '9',  '0',  '-',  '=',    8,    9,   // 0
         'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',
         'o',  'p',  '[',  ']',   13,  CTL,  'a',  's',   // 1
         'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',
        '\'',  '`',   SH, '\\',  'z',  'x',  'c',  'v',   // 2
         'b',  'n',  'm',  ',',  '.',  '/',   SH,   PS,
         ALT,  ' ',   CL,   F1,   F2,   F3,   F4,   F5,   // 3
          F6,   F7,   F8,   F9,  F10,   NL,   SL,   KH,
          KU,  KPU,   KM,   KL,   KC,   KR,   KP,   KE,   // 4
          KD,  KPD,   KI,  KDL,  INV,  INV,  INV,  F11,
         F12,  INV,  INV,  INV,  INV,  INV,  INV,  INV,   // 5
         INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV,
         INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV,   // 6
         INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV,
         INV,  INV,  INV,  INV,  INV,  INV,  INV,  INV,   // 7
    },
};

//----------------------------------------------------------------------------
//  @struct     kbstate
/// @brief      Holds the current state of the keyboard.
//----------------------------------------------------------------------------
typedef struct
kbstate
{
    scanmap_t   scanmap;        ///< The current keyboard scancode map
    uint8_t     meta;           ///< Mask of meta keys currently pressed.
    uint8_t     toggle;         ///< Mask of toggle keys currently pressed.
    uint8_t     lastscancode;   ///< Last scan code returned by the keyboard.
    uint8_t     pad;
    uint8_t     buf_head;       ///< Index of oldest buffer character.
    uint8_t     buf_tail;       ///< Index of next empty buffer slot.
    char        buffer[BUFSIZ]; ///< Buffer to hold recently typed chars.
} kbstate_t;

/// Current keyboard state.
static kbstate_t state;

//----------------------------------------------------------------------------
//  @function   toggle
/// @brief      Toggle the requested toggle flag.
/// @param[in]  toggle  The toggle flag to toggle.
//----------------------------------------------------------------------------
static inline
void toggle(uint8_t toggle)
{
    if (state.toggle & toggle)
        state.toggle &= ~toggle;
    else
        state.toggle |= toggle;
}

//----------------------------------------------------------------------------
//  @function   addbuf
/// @brief      Add a character to the keyboard input buffer.
/// @details    If the buffer is full, the character is discarded.
/// @param[in]  ch      The ascii character to add to the buffer.
//----------------------------------------------------------------------------
static void
addbuf(char ch)
{
    // Is the buffer full?
    if ((state.buf_tail + 1) % BUFSIZ == state.buf_head)
        return;

    // Add the character to the tail of the buffer.
    state.buffer[state.buf_tail++] = ch;
    if (state.buf_tail == BUFSIZ)
        state.buf_tail = 0;
}

//----------------------------------------------------------------------------
//  @function   isr_keyboard
/// @brief      Interrupt service routine for keyboard IRQ 1.
/// @param[in]  interrupt   Interrupt number (unused).
/// @param[in]  error       Interrupt error code (unused).
//----------------------------------------------------------------------------
static void
isr_keyboard(uint8_t interrupt, uint64_t error)
{
    (void)interrupt;
    (void)error;

    // Get the scan code and the break state (key up or key down).
    uint8_t code  = io_inb(KB_PORT_DATA);
    bool    keyup = !!(code & 0x80);

    // Store the last returned scan code.
    state.lastscancode = code;

    // Chop off the break bit.
    code &= ~0x80;

    // Get the shifted state.
    bool shifted = !!(state.meta & META_SHIFT);

    // Handle capslock.
    if (state.toggle & TOGGLE_CAPSLOCK) {
        uint8_t key = state.scanmap.unshifted[code];
        if (key >= 'a' && key <= 'z')
            shifted = !shifted;
    }

    // Convert the scan code to a key code.
    uint8_t key = shifted ?
                  state.scanmap.shifted[code] :
                  state.scanmap.unshifted[code];

    // key up?
    if (keyup) {
        switch (key) {
            case KEY_SHIFT:
                state.meta &= ~META_SHIFT;
                break;
            case KEY_CTRL:
                state.meta &= ~META_CTRL;
                break;
            case KEY_ALT:
                state.meta &= ~META_ALT;
                break;
            case KEY_CAPSLOCK:
                toggle(TOGGLE_CAPSLOCK);
                break;
            case KEY_NUMLOCK:
                toggle(TOGGLE_NUMLOCK);
                break;
            case KEY_SCRLOCK:
                toggle(TOGGLE_SCRLOCK);
                break;
        }
    }

    // key down?
    else {
        switch (key) {
            case KEY_SHIFT:
                state.meta |= META_SHIFT;
                break;
            case KEY_CTRL:
                state.meta |= META_CTRL;
                break;
            case KEY_ALT:
                state.meta |= META_ALT;
                break;
        }

        // Convert the key to a character.
        if (key < 0x80) {
            switch (state.meta & (META_CTRL | META_ALT)) {
                case 0:
                    addbuf((char)key);
                    break;
                case META_CTRL:
                    if (key >= 'a' && key <= 'z')
                        addbuf((char)(key - 'a' + 1));
                    else if (key >= 'A' && key <= 'Z')
                        addbuf((char)(key - 'A' + 1));
                    break;
            }
        }
    }

    // Send the end-of-interrupt signal.
    io_outb(PIC_PORT_CMD_MASTER, PIC_CMD_EOI);
}

//----------------------------------------------------------------------------
//  @function   kb_init
//  @brief      Initialize the keyboard so that it can provide input to the
//              kernel.
//----------------------------------------------------------------------------
void
kb_init()
{
    // Initialize keyboard state.
    memcpy(&state.scanmap, &ps2, sizeof(state.scanmap));
    state.meta = 0;
    state.toggle = 0;
    state.lastscancode = 0;
    state.buf_head = state.buf_tail = 0;
    memzero(&state.buffer, sizeof(state.buffer));

    // Assign the interrupt service routine.
    isr_set(0x21, isr_keyboard);

    // Enable the keyboard hardware interrupt (IRQ1).
    irq_enable(1);
}

//----------------------------------------------------------------------------
//  @function   kb_getchar
//  @brief      Return the next character from the keyboard's input buffer.
//  @details    If the buffer is empty, return 0.
//  @returns    The ascii value of the next character in the input buffer,
//              or 0 if there are no characters available.
//----------------------------------------------------------------------------
char
kb_getchar() {
    // Buffer empty?
    if (state.buf_tail == state.buf_head)
        return 0;

    // Pull the next character from the head of the buffer.
    char ch = state.buffer[state.buf_head++];
    if (state.buf_head == BUFSIZ)
        state.buf_head = 0;
    return ch;
}

//----------------------------------------------------------------------------
//  @function   kb_lastscancode
//  @brief      Returns the last scan code received by the keyboard.
//  @returns    Last scan code received by the keyboard.
//----------------------------------------------------------------------------
uint8_t
kb_lastscancode()
{
    return state.lastscancode;
}

//----------------------------------------------------------------------------
//  @function   kb_meta
//  @brief      Return the meta-key bit mask.
//  @returns    The meta-key bitmask.
//----------------------------------------------------------------------------
uint8_t
kb_meta()
{
    return state.meta;
}

//----------------------------------------------------------------------------
//  @function   kb_toggle
//  @brief      Return the toggle-key bit mask.
//  @returns    The toggle-key bitmask.
//----------------------------------------------------------------------------
uint8_t
kb_toggle()
{
    return state.toggle;
}

//----------------------------------------------------------------------------
//  @function   kb_installscanmap
//  @brief      Install a new keyboard scan map.
//  @param[in]  map     The scan map to install.
//----------------------------------------------------------------------------
void
kb_installscanmap(scanmap_t *map)
{
    memcpy(&state.scanmap, map, sizeof(state.scanmap));
}
