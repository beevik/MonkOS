//============================================================================
/// @file       debug.c
/// @brief      Debugging helpers.
//
//  Copyright 2016 Brett Vickers.
//  Use of this source code is governed by a BSD-style license
//  that can be found in the MonkOS LICENSE file.
//============================================================================

#include <libc/stdio.h>
#include <kernel/debug.h>

static const char digit[] = "0123456789abcdef";

int
debug_dump_registers(char *buf, size_t bufsize, const registers_t *regs)
{
    return snprintf(
        buf, bufsize,
        "RAX: %016lx    RSI: %016lx    R11: %016lx\n"
        "RBX: %016lx    RDI: %016lx    R12: %016lx\n"
        "RCX: %016lx     R8: %016lx    R13: %016lx\n"
        "RDX: %016lx     R9: %016lx    R14: %016lx\n"
        "RBP: %016lx    R10: %016lx    R15: %016lx\n",
        regs->rax, regs->rsi, regs->r11,
        regs->rbx, regs->rdi, regs->r12,
        regs->rcx, regs->r8, regs->r13,
        regs->rdx, regs->r9, regs->r14,
        regs->rbp, regs->r10, regs->r15);
}

int
debug_dump_cpuflags(char *buf, size_t bufsize, uint64_t rflags)
{
#define B(F)    ((rflags & F) ? 1 : 0)

    return snprintf(
        buf, bufsize,
        "CF=%u   PF=%u   AF=%u   ZF=%u   SF=%u   "
        "TF=%u   IF=%u   DF=%u   OF=%u   IOPL=%u\n",
        B(CPU_EFLAGS_CARRY), B(CPU_EFLAGS_PARITY), B(CPU_EFLAGS_ADJUST),
        B(CPU_EFLAGS_ZERO), B(CPU_EFLAGS_SIGN), B(CPU_EFLAGS_TRAP),
        B(CPU_EFLAGS_INTERRUPT), B(CPU_EFLAGS_DIRECTION),
        B(CPU_EFLAGS_OVERFLOW), (rflags >> 12) & 3);

#undef B
}

int
debug_dump_memory(char *buf, size_t bufsize, const void *mem, size_t memsize)
{
    char          *b  = buf;
    char          *bt = buf + bufsize;
    const uint8_t *m  = (const uint8_t *)mem;
    const uint8_t *mt = (const uint8_t *)mem + memsize;

    while (b < bt && m < mt) {

        // Dump up to 16 hexadecimal byte values separated by spaces.
        for (int j = 0; j < 16; j++) {
            if (m + j < mt) {
                if (b + 3 < bt) {
                    uint8_t v = m[j];
                    b[0] = digit[v >> 4];
                    b[1] = digit[v & 0xf];
                    b[2] = ' ';
                }
            }
            else if (b + 3 < bt) {
                b[0] = b[1] = b[2] = ' ';
            }
            b += 3;

            // Add a 1-space gutter between each group of 4 bytes.
            if (((j + 1) & 3) == 0) {
                if (b + 1 < bt) {
                    *b = ' ';
                }
                b++;
            }
        }

        // Dump a 3-space gutter.
        if (b + 3 < bt) {
            b[0] = b[1] = b[2] = ' ';
        }
        b += 3;

        // Dump up to 16 ASCII bytes.
        for (int j = 0; j < 16; j++) {
            if (m + j < mt) {
                if (b < bt) {
                    uint8_t v = m[j];
                    *b = (v < 32 || v > 126) ? '.' : (char)v;
                }
            }
            else if (b + 1 < bt) {
                *b = ' ';
            }
            b++;
        }

        // Dump a carriage return.
        if (b + 1 < bt) {
            *b = '\n';
        }
        b++;

        m += 16;
    }

    // Null-terminate the buffer.
    if (b < bt) {
        *b = 0;
    }
    else if (bufsize > 0) {
        b[bufsize - 1] = 0;
    }

    return (int)(bt - b);
}
