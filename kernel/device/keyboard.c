//============================================================================
/// @file       keyboard.c
/// @brief      Keyboard input routines.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#include <core.h>
#include <libc/string.h>
#include <kernel/cpu.h>
#include <kernel/device/keyboard.h>
#include <kernel/interrupt/interrupt.h>

// Keyboard I/O ports
#define KB_PORT_DATA    0x60     ///< Keyboard I/O data port.

// Other constants
#define MAX_BUFSIZ    32         ///< Keyboard input buffer size.

// Key code abbreviations, used to set up the default scan map table below.
#define BSP    KEY_BACKSPACE
#define TAB    KEY_TAB
#define ENT    KEY_ENTER
#define ESC    KEY_ESCAPE
#define CTL    KEY_CTRL
#define SHF    KEY_SHIFT
#define ALT    KEY_ALT
#define PSC    KEY_PRTSCR
#define CLK    KEY_CAPSLOCK
#define NLK    KEY_NUMLOCK
#define SLK    KEY_SCRLOCK
#define KIN    KEY_INSERT
#define KEN    KEY_END
#define KDN    KEY_DOWN
#define KPD    KEY_PGDN
#define KLT    KEY_LEFT
#define KCT    KEY_CENTER
#define KRT    KEY_RIGHT
#define KHM    KEY_HOME
#define KUP    KEY_UP
#define KPU    KEY_PGUP
#define KDL    KEY_DEL
#define KMI    KEY_MINUS
#define KPL    KEY_PLUS
#define F_1    KEY_F1
#define F_2    KEY_F2
#define F_3    KEY_F3
#define F_4    KEY_F4
#define F_5    KEY_F5
#define F_6    KEY_F6
#define F_7    KEY_F7
#define F_8    KEY_F8
#define F_9    KEY_F9
#define F10    KEY_F10
#define F11    KEY_F11
#define F12    KEY_F12
#define SES    KEY_SCANESC
#define INV    KEY_INVALID
#define APO    '\''
#define BSL    '\\'

/// US English PSC/2 keyboard scan map (default setting)
static const keylayout_t ps2_layout =
{
    .shifted =
    {
        INV, ESC, '!', '@', '#', '$', '%', '^',
        '&', '*', '(', ')', '_', '+', BSP, TAB, // 0
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
        'O', 'P', '{', '}', ENT, CTL, 'A', 'S', // 1
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
        '"', '~', SHF, '|', 'Z', 'X', 'C', 'V', // 2
        'B', 'N', 'M', '<', '>', '?', SHF, PSC,
        ALT, ' ', CLK, F_1, F_2, F_3, F_4, F_5, // 3
        F_6, F_7, F_8, F_9, F10, NLK, SLK, KHM,
        KUP, KPU, KMI, KLT, KCT, KRT, KPL, KEN, // 4
        KDN, KPD, KIN, KDL, INV, INV, INV, F11,
        F12, INV, INV, INV, INV, INV, INV, INV, // 5
        SES, SES, INV, INV, INV, INV, INV, INV,
        INV, INV, INV, INV, INV, INV, INV, INV, // 6
        INV, INV, INV, INV, INV, INV, INV, INV,
        INV, INV, INV, INV, INV, INV, INV, INV, // 7
    },
    .unshifted =
    {
        INV, ESC, '1', '2', '3', '4', '5', '6',
        '7', '8', '9', '0', '-', '=', BSP, TAB, // 0
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
        'o', 'p', '[', ']', ENT, CTL, 'a', 's', // 1
        'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
        APO, '`', SHF, BSL, 'z', 'x', 'c', 'v', // 2
        'b', 'n', 'm', ',', '.', '/', SHF, PSC,
        ALT, ' ', CLK, F_1, F_2, F_3, F_4, F_5, // 3
        F_6, F_7, F_8, F_9, F10, NLK, SLK, KHM,
        KUP, KPU, KMI, KLT, KCT, KRT, KPL, KEN, // 4
        KDN, KPD, KIN, KDL, INV, INV, INV, F11,
        F12, INV, INV, INV, INV, INV, INV, INV, // 5
        SES, SES, INV, INV, INV, INV, INV, INV,
        INV, INV, INV, INV, INV, INV, INV, INV, // 6
        INV, INV, INV, INV, INV, INV, INV, INV,
        INV, INV, INV, INV, INV, INV, INV, INV, // 7
    },
};

/// Keyboard state.
struct kbstate
{
    keylayout_t  layout;         ///< The installed keyboard layout.
    uint8_t      meta;           ///< Mask of meta keys currently pressed.
    uint8_t      buf_head;       ///< Index of oldest key in buf.
    uint8_t      buf_tail;       ///< Index of next empty slot in buf.
    atomic_uchar buf_size;       ///< Number of keys in the buf.
    key_t        buf[MAX_BUFSIZ]; ///< Buffer holding unconsumed keys.
};

typedef struct kbstate kbstate_t;

/// Current keyboard state.
static kbstate_t state;

static inline void
toggle(uint8_t flag)
{
    if (state.meta & flag) {
        state.meta &= ~flag;
    }
    else {
        state.meta |= flag;
    }
}

static void
addkey(uint8_t brk, uint8_t meta, uint8_t code, uint8_t ch)
{
    // Reset the scan code escape state whenever a new key is added to the
    // buffer.
    state.meta &= ~META_ESCAPED;

    // Is the buffer full?
    // There is no need for an atomic comparison here, because the ISR
    // function calling addkey can never be interrupted by anything that
    // touches the buffer.
    if (state.buf_size == MAX_BUFSIZ) {
        return;
    }

    key_t key =
    {
        .brk  = brk,
        .meta = meta,
        .code = code,
        .ch   = ch
    };

    // Add the character to the tail of the buffer.
    state.buf[state.buf_tail++] = key;
    if (state.buf_tail == MAX_BUFSIZ) {
        state.buf_tail = 0;
    }

    // state.buf_size++;
    atomic_fetch_add_explicit(&state.buf_size, 1, memory_order_relaxed);
}

static void
isr_keyboard(const interrupt_context_t *context)
{
    (void)context;

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
        if ((ukeycode >= 'a') && (ukeycode <= 'z')) {
            shifted = !shifted;
        }
    }

    // Convert the scan code to a properly shifted key code.
    uint8_t keycode = shifted ? state.layout.shifted[scancode] : ukeycode;

    // Key up?
    if (keyup) {
        switch (keycode)
        {
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
        switch (keycode)
        {
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
            switch (state.meta & (META_CTRL | META_ALT))
            {
                case 0:
                    ch = (char)keycode;
                    break;

                case META_CTRL:
                    if ((ukeycode >= 'a') && (ukeycode <= 'z')) {
                        ch = (char)(ukeycode - 'a' + 1);
                    }
                    break;
            }
        }
        addkey(1, state.meta, ukeycode, ch);
    }

done:
    // Send the end-of-interrupt signal.
    io_outb(PIC_PORT_CMD_MASTER, PIC_CMD_EOI);
}

void
kb_init()
{
    // Default to the PSC/2 keyboard layout.
    memcpy(&state.layout, &ps2_layout, sizeof(state.layout));

    // Initialize keyboard state.
    state.meta     = 0;
    state.buf_head = 0;
    state.buf_tail = 0;
    state.buf_size = 0;
    memzero(&state.buf, sizeof(state.buf));

    // Assign the interrupt service routine.
    isr_set(TRAP_IRQ_KEYBOARD, isr_keyboard);

    // Enable the keyboard hardware interrupt (IRQ1).
    irq_enable(IRQ_KEYBOARD);
}

void
kb_setlayout(keylayout_t *layout)
{
    memcpy(&state.layout, layout, sizeof(state.layout));
}

char
kb_getchar()
{
    for (;;) {
        // Buffer empty? (state.buf_size == 0?) Check atomically because
        // this function could be interrupted by the keyboard ISR.
        uint8_t size = 0;
        if (atomic_compare_exchange_strong(&state.buf_size, &size, 0)) {
            return 0;
        }

        // Pull the next character from the head of the buffer.
        char ch = state.buf[state.buf_head++].ch;
        if (state.buf_head == MAX_BUFSIZ) {
            state.buf_head = 0;
        }

        // state.buf_size--
        atomic_fetch_sub_explicit(&state.buf_size, 1, memory_order_relaxed);

        // Valid character?
        if (ch != 0) {
            return ch;
        }
    }
}

bool
kb_getkey(key_t *key)
{
    // Buffer empty? (state.buf_size == 0?) Check atomically because this
    // function could be interrupted by the keyboard ISR.
    uint8_t size = 0;
    if (atomic_compare_exchange_strong(&state.buf_size, &size, 0)) {
        *(uint32_t *)key = 0;
        return false;
    }

    // Pull the next character from the head of the buffer.
    *key = state.buf[state.buf_head++];
    if (state.buf_head == MAX_BUFSIZ) {
        state.buf_head = 0;
    }

    // state.buf_size--
    atomic_fetch_sub_explicit(&state.buf_size, 1, memory_order_relaxed);

    return true;
}

uint8_t
kb_meta()
{
    return state.meta;
}
