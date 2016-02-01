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
#include <kernel/interrupt.h>
#include <kernel/io.h>
#include <kernel/keyboard.h>
#include <libc/string.h>

// Keyboard I/O ports
#define KB_PORT_DATA    0x60    ///< Keyboard I/O data port.

// Other constants
#define BUFSIZ          32      ///< Keyboard input buffer size.

// Key code abbreviations, used to set up the default scan map table below.
#define CTL             KEY_CTRL
#define SH              KEY_SHIFT
#define ALT             KEY_ALT
#define PS              KEY_PRTSCR
#define CL              KEY_CAPSLOCK
#define NL              KEY_NUMLOCK
#define SL              KEY_SCRLOCK
#define KI              KEY_INSERT
#define KE              KEY_END
#define KD              KEY_DOWN
#define KPD             KEY_PGDN
#define KL              KEY_LEFT
#define KC              KEY_CENTER
#define KR              KEY_RIGHT
#define KH              KEY_HOME
#define KU              KEY_UP
#define KPU             KEY_PGUP
#define KDL             KEY_DEL
#define KM              KEY_MINUS
#define KP              KEY_PLUS
#define F1              KEY_F1
#define F2              KEY_F2
#define F3              KEY_F3
#define F4              KEY_F4
#define F5              KEY_F5
#define F6              KEY_F6
#define F7              KEY_F7
#define F8              KEY_F8
#define F9              KEY_F9
#define F10             KEY_F10
#define F11             KEY_F11
#define F12             KEY_F12
#define SE              KEY_SCANESC
#define INV             KEY_INVALID

/// US English PS/2 keyboard scan map (default setting)
static const keylayout_t ps2_layout =
{
    .shifted   =
    {
        INV,  27, '!', '@', '#', '$', '%', '^',
        '&', '*', '(', ')', '_', '+',   8,   9,     // 0
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
        'O', 'P', '{', '}',  13, CTL, 'A', 'S',     // 1
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
        '"', '~', SH,  '|', 'Z', 'X', 'C', 'V',     // 2
        'B', 'N', 'M', '<', '>', '?', SH,  PS,
        ALT, ' ', CL,  F1,  F2,  F3,  F4,  F5,      // 3
        F6,  F7,  F8,  F9,  F10, NL,  SL,  KH,
        KU,  KPU, KM,  KL,  KC,  KR,  KP,  KE,      // 4
        KD,  KPD, KI,  KDL, INV, INV, INV, F11,
        F12, INV, INV, INV, INV, INV, INV, INV,     // 5
        SE,  SE,  INV, INV, INV, INV, INV, INV,
        INV, INV, INV, INV, INV, INV, INV, INV,     // 6
        INV, INV, INV, INV, INV, INV, INV, INV,
        INV, INV, INV, INV, INV, INV, INV, INV,     // 7
    },
    .unshifted =
    {
        INV,   27, '1', '2',  '3', '4', '5', '6',
        '7',  '8', '9', '0',  '-', '=',   8,   9,   // 0
        'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
        'o',  'p', '[', ']',   13, CTL, 'a', 's',   // 1
        'd',  'f', 'g', 'h',  'j', 'k', 'l', ';',
        '\'', '`', SH,  '\\', 'z', 'x', 'c', 'v',   // 2
        'b',  'n', 'm', ',',  '.', '/', SH,  PS,
        ALT,  ' ', CL,  F1,   F2,  F3,  F4,  F5,    // 3
        F6,   F7,  F8,  F9,   F10, NL,  SL,  KH,
        KU,   KPU, KM,  KL,   KC,  KR,  KP,  KE,    // 4
        KD,   KPD, KI,  KDL,  INV, INV, INV, F11,
        F12,  INV, INV, INV,  INV, INV, INV, INV,   // 5
        SE,   SE,  INV, INV,  INV, INV, INV, INV,
        INV,  INV, INV, INV,  INV, INV, INV, INV,   // 6
        INV,  INV, INV, INV,  INV, INV, INV, INV,
        INV,  INV, INV, INV,  INV, INV, INV, INV,   // 7
    },
};

//----------------------------------------------------------------------------
//  @struct     kbstate
/// @brief      Holds the current state of the keyboard.
//----------------------------------------------------------------------------
typedef struct kbstate
{
    keylayout_t layout;         ///< The installed keyboard layout.
    uint8_t     meta;           ///< Mask of meta keys currently pressed.
    uint8_t     buf_head;       ///< Index of oldest key in keybuf.
    uint8_t     buf_tail;       ///< Index of next empty slot in keybuf.
    key_t       keybuf[BUFSIZ]; ///< Buffer holding unconsumed keys.
} kbstate_t;

/// Current keyboard state.
static kbstate_t state;

//----------------------------------------------------------------------------
//  @function   toggle
/// @brief      Toggle the requested meta flag.
/// @param[in]  flag    The meta flag to toggle.
//----------------------------------------------------------------------------
static inline void
toggle(uint8_t flag)
{
    if (state.meta & flag)
        state.meta &= ~flag;
    else
        state.meta |= flag;
}

//----------------------------------------------------------------------------
//  @function   addkey
/// @brief      Add a key record to the keyboard input buffer.
/// @details    If the buffer is full, the character is discarded.
/// @param[in]  brk     The break code (0 = key up, 1 = key down).
/// @param[in]  meta    Metakey state.
/// @param[in]  code    The unshifted keycode value.
/// @param[in]  ch      Character value, if valid.
//----------------------------------------------------------------------------
static void
addkey(uint8_t brk, uint8_t meta, uint8_t code, uint8_t ch)
{
    // Reset the scan code escape state whenever a new key is added to the
    // buffer.
    state.meta &= ~META_ESCAPED;

    // Is the buffer full?
    if ((state.buf_tail + 1) % BUFSIZ == state.buf_head)
        return;

    key_t key =
    {
        .brk  = brk,
        .meta = meta,
        .code = code,
        .ch   = ch
    };

    // Add the character to the tail of the buffer.
    state.keybuf[state.buf_tail++] = key;
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
    uint8_t scancode = io_inb(KB_PORT_DATA);
    bool    keyup    = !!(scancode & 0x80);

    // Chop off the break bit.
    scancode &= ~0x80;

    // Get the shifted state.
    bool shifted = !!(state.meta & META_SHIFT);

    // Convert the scan code into an unshifted key code.
    uint8_t ukeycode = state.layout.unshifted[scancode];

    // Is the key a keyboard scan escape code? If so, don't add it to the
    // buffer, but track the escape as a meta-state.
    if (ukeycode == KEY_SCANESC) {
        state.meta |= META_ESCAPED;
        goto done;
    }

    // Alter shift state based on capslock state.
    if (state.meta & META_CAPSLOCK) {
        if ((ukeycode >= 'a') && (ukeycode <= 'z'))
            shifted = !shifted;
    }

    // Convert the scan code to a properly shifted key code.
    uint8_t keycode = shifted ? state.layout.shifted[scancode] : ukeycode;

    // Key up?
    if (keyup) {
        switch (keycode) {
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
                toggle(META_CAPSLOCK);
                break;

            case KEY_NUMLOCK:
                toggle(META_NUMLOCK);
                break;

            case KEY_SCRLOCK:
                toggle(META_SCRLOCK);
                break;
        }
        addkey(0, state.meta, ukeycode, 0);
    }
    // Key down?
    else {
        switch (keycode) {
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
        char ch = 0;
        if (keycode < 0x80) {
            switch (state.meta & (META_CTRL | META_ALT)) {
                case 0:
                    ch = (char)keycode;
                    break;

                case META_CTRL:
                    if ((ukeycode >= 'a') && (ukeycode <= 'z'))
                        ch = (char)(ukeycode - 'a' + 1);
                    break;
            }
        }
        addkey(1, state.meta, ukeycode, ch);
    }

done:
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
    // Default to the PS/2 keyboard layout.
    memcpy(&state.layout, &ps2_layout, sizeof(state.layout));

    // Initialize keyboard state.
    state.meta     = 0;
    state.buf_head = 0;
    state.buf_tail = 0;
    memzero(&state.keybuf, sizeof(state.keybuf));

    // Assign the interrupt service routine.
    isr_set(0x21, isr_keyboard);

    // Enable the keyboard hardware interrupt (IRQ1).
    irq_enable(1);
}

//----------------------------------------------------------------------------
//  @function   kb_setlayout
//  @brief      Install a new keyboard layout.
//  @param[in]  layout  The keyboard layout to install.
//----------------------------------------------------------------------------
void
kb_setlayout(keylayout_t *layout)
{
    memcpy(&state.layout, layout, sizeof(state.layout));
}

//----------------------------------------------------------------------------
//  @function   kb_getchar
//  @brief      Return the next available character from the keyboard's
//              input buffer. Onl
//  @details    If the buffer is empty, return 0.
//  @returns    The ascii value of the next character in the input buffer,
//              or 0 if there are no characters available.
//----------------------------------------------------------------------------
char
kb_getchar()
{
    char ch = 0;

    // To prevent race conditions while accessing buffer indexes, disable
    // interrupts. Preserve the interrupt flag so we can restore it before
    // returning.
    asm volatile ("pushfq");
    asm volatile ("cli");

    for (;;) {

        // Buffer empty?
        if (state.buf_tail == state.buf_head)
            break;

        // Pull the next character from the head of the buffer.
        ch = state.keybuf[state.buf_head++].ch;
        if (state.buf_head == BUFSIZ)
            state.buf_head = 0;

        // Valid character?
        if (ch != 0)
            break;
    }

    // Restore the interrupt flag.
    asm volatile ("popfq");
    return ch;
}

//----------------------------------------------------------------------------
//  @function   kb_getkey
//  @brief      Return the next key from the keyboard's input buffer.
//  @details    If the buffer is empty, return 0.
//  @param[out] key     The key record of the next key in the buffer.
//  @returns    true if there is a key in the buffer, false otherwise.
//----------------------------------------------------------------------------
bool
kb_getkey(key_t *key)
{
    *(uint32_t *)key = 0;
    bool found = false;

    // To prevent race conditions while accessing buffer indexes, disable
    // interrupts. Preserve the interrupt flag so we can restore it before
    // returning.
    asm volatile ("pushfq");
    asm volatile ("cli");

    // Buffer empty?
    if (state.buf_tail == state.buf_head)
        goto done;

    found = true;

    // Pull the next character from the head of the buffer.
    *key = state.keybuf[state.buf_head++];
    if (state.buf_head == BUFSIZ)
        state.buf_head = 0;

done:
    // Restore the interrupt flag.
    asm volatile ("popfq");
    return found;
}

//----------------------------------------------------------------------------
//  @function   kb_meta
//  @brief      Return the current meta-key bit mask.
//  @returns    The meta-key bitmask.
//----------------------------------------------------------------------------
uint8_t
kb_meta()
{
    return state.meta;
}
