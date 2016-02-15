//============================================================================
/// @file       exception.c
/// @brief      CPU exceptions.
//
//  Copyright 2016 Brett Vickers.
//  Use of this source code is governed by a BSD-style license
//  that can be found in the MonkOS LICENSE file.
//============================================================================

#include <core.h>
#include <libc/stdio.h>
#include <kernel/debug/dump.h>
#include <kernel/device/tty.h>
#include <kernel/interrupt/exception.h>
#include <kernel/interrupt/interrupt.h>

static const char *exceptionstr[] =
{
    "#DE: Divide by zero exception",
    "#DB: Debug exception",
    "Non-maskable interrupt",
    "#BP: Breakpoint exception",
    "#OF: Overflow exception",
    "#BR: BOUND Range exceeded exception",
    "#UD: Invalid opcode exception",
    "#NM: Device not available exception",
    "#DF: Double fault exception",
    "Coprocessor segment overrun",
    "#TS: Invalid TSS exception",
    "#NP: Segment not present exception",
    "#SS: Stack fault exception",
    "#GP: General protection exception",
    "#PF: Page fault exception",
    "Unknown exception",
    "#MF: x87 FPU floating-point error",
    "#AC: Alignment check exception",
    "#MC: Machine-check exception",
    "#XM: SIMD floating-point exception",
    "#VE: Virtualization exception",
};

static void
dump_context(int id, const interrupt_context_t *context)
{
    tty_printf(
        id,
        "INT: %02x   Error: %08x\n\n",
        context->interrupt, context->error);
    tty_printf(
        id,
        "CS:RIP: %04x:%016lx             SS:RSP: %04x:%016lx\n\n",
        context->cs, context->retaddr, context->ss, context->rsp);

    char buf[640];

    dump_registers(buf, sizeof(buf), &context->regs);
    tty_print(id, buf);
    tty_print(id, "\n");

    dump_cpuflags(buf, sizeof(buf), context->rflags);
    tty_print(id, buf);
    tty_print(id, "\n");

    tty_print(id, "Stack:\n");
    void *stack = (void *)context->rsp;
    dump_memory(buf, sizeof(buf), stack, 8 * 16, DUMPSTYLE_ADDR);
    tty_print(id, buf);
}

static void
hang()
{
    for (;;) {
        interrupts_disable();
        halt();
    }
}

static void
isr_fatal(const interrupt_context_t *context)
{
    int i = context->interrupt;

    const char *exstr = i < arrsize(exceptionstr)
                        ? exceptionstr[i] : "Unknown exception.";

    tty_activate(0);
    tty_set_textcolor(0, TEXTCOLOR_WHITE, TEXTCOLOR_RED);
    tty_clear(0);
    tty_printf(0, "%s\n\n", exstr);

    dump_context(0, context);

    hang();
}

static void
isr_breakpoint(const interrupt_context_t *context)
{
    (void)context;

    tty_print(0, "Breakpoint hit.\n");
}

void
exceptions_init()
{
    for (int i = 0; i < 32; i++)
        isr_set(i, isr_fatal);   // fatal for now. temporary.
    isr_set(0xff, isr_fatal);

    isr_set(EXCEPTION_BREAKPOINT, isr_breakpoint);
}
