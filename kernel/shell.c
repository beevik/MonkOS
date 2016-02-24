//============================================================================
/// @file       shell.c
/// @brief      Simple kernel shell for testing purposes.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#include <core.h>
#include <libc/stdio.h>
#include <libc/stdlib.h>
#include <libc/string.h>
#include <kernel/device/tty.h>
#include <kernel/device/keyboard.h>
#include <kernel/mem/acpi.h>
#include <kernel/x86/cpu.h>

#define TTY_CONSOLE  0

/*
 *  Shell handler modes
 *
 */

typedef struct mode
{
    void (*start)();
    void (*run)();
    void (*stop)();
} mode_t;

// Interactive shell mode
static void
shell_prompt();
static void
shell_run();

static mode_t mode_shell =
{
    shell_prompt,
    shell_run,
    NULL
};

// Keyboard key-code display mode
static void
keycode_run();

static mode_t mode_keycode =
{
    NULL,
    keycode_run,
    NULL
};

static mode_t *active_mode = &mode_shell;

static void
switch_mode(mode_t *mode)
{
    if (active_mode->stop != NULL)
        active_mode->stop();

    active_mode = mode;

    if (active_mode->start != NULL)
        active_mode->start();
}

/*
 *  Interactive shell commands
 *
 */

static bool
cmd_display_help();
static bool
cmd_display_apic();
static bool
cmd_display_pcie();
static bool
cmd_switch_to_keycodes();

struct cmd
{
    const char *str;
    const char *help;
    bool        (*run)();
};

static struct cmd commands[] =
{
    { "?", NULL, cmd_display_help },
    { "help", "Show this help text", cmd_display_help },
    { "apic", "Show APIC configuration", cmd_display_apic },
    { "pcie", "Show PCIexpress configuration", cmd_display_pcie },
    { "kc", "Switch to keycode display mode", cmd_switch_to_keycodes },
};

static int
cmp_cmds(const void *c1, const void *c2)
{
    return strcmp(((const struct cmd *)c1)->str,
                  ((const struct cmd *)c2)->str);
}

static void
sort_cmds()
{
    qsort(commands, arrsize(commands), sizeof(struct cmd), cmp_cmds);
}

static bool
cmd_display_help()
{
    tty_print(TTY_CONSOLE, "Available commands:\n");
    for (int i = 0; i < arrsize(commands); i++) {
        if (commands[i].help == NULL)
            continue;
        tty_printf(TTY_CONSOLE, "  %-8s %s\n",
                   commands[i].str, commands[i].help);
    }
    return true;
}

static bool
cmd_display_apic()
{
    const struct acpi_madt *madt = acpi_madt();
    if (madt == NULL) {
        tty_print(TTY_CONSOLE, "No ACPI MADT detected.\n");
        return true;
    }

    tty_printf(TTY_CONSOLE, "Local APIC addr: %#x\n", madt->ptr_local_apic);

    const struct acpi_madt_local_apic *local = NULL;
    while ((local = acpi_next_local_apic(local)) != NULL) {
        tty_printf(TTY_CONSOLE, "Local APIC id %u: %s\n",
                   local->apicid,
                   (local->flags & 1) ? "Usable" : "Unusable");
    }

    const struct acpi_madt_io_apic *io = NULL;
    while ((io = acpi_next_io_apic(io)) != NULL) {
        tty_printf(TTY_CONSOLE, "I/O APIC id %u: Addr=%#x Base=%u\n",
                   io->apicid,
                   io->ptr_io_apic,
                   io->interrupt_base);
    }

    const struct acpi_madt_iso *iso = NULL;
    while ((iso = acpi_next_iso(iso)) != NULL) {
        tty_printf(TTY_CONSOLE, "ISO irq=%-2u int=%-2u flags=0x%04x\n",
                   iso->source,
                   iso->interrupt,
                   iso->flags);
    }

    return true;
}

static bool
cmd_display_pcie()
{
    const struct acpi_mcfg_addr *addr = acpi_next_mcfg_addr(NULL);
    if (addr == NULL) {
        tty_print(TTY_CONSOLE, "No PCIe configuration.\n");
        return true;
    }

    while (addr != NULL) {
        tty_printf(TTY_CONSOLE, "PCIe addr=0x%08x  grp=%-2u bus=%02x..%02x\n",
                   addr->base, addr->seg_group, addr->bus_start,
                   addr->bus_end);
        addr = acpi_next_mcfg_addr(addr);
    }

    return true;
}

static bool
cmd_switch_to_keycodes()
{
    tty_print(TTY_CONSOLE,
              "Entering keycode mode. Hit Alt-Tab to exit.\n");
    switch_mode(&mode_keycode);
    return false;
}

static bool
shell_exec(const char *cmd)
{
    for (int i = 0; i < arrsize(commands); i++) {
        if (!strcmp(commands[i].str, cmd))
            return commands[i].run();
    }

    tty_printf(TTY_CONSOLE, "Unknown command: %s\n", cmd);
    return true;
}

static void
shell_prompt()
{
    tty_print(TTY_CONSOLE, "> ");
}

static void
shell_run()
{
    char cmd[256];
    int  cmdlen = 0;

    for (;;) {
        halt();

        key_t key;
        bool  avail;
        while ((avail = kb_getkey(&key)) != false) {
            if (key.ch >= 32 && key.ch < 127) {
                if (cmdlen < arrsize(cmd) - 1) {
                    cmd[cmdlen] = key.ch;
                    tty_printc(TTY_CONSOLE, cmd[cmdlen]);
                    cmdlen++;
                }
            }
            else if (key.brk == KEYBRK_DOWN) {
                if (key.code == KEY_ENTER) {
                    tty_printc(TTY_CONSOLE, '\n');
                    cmd[cmdlen] = 0;
                    bool cont = shell_exec(cmd);
                    cmdlen = 0;
                    if (cont)
                        shell_prompt();
                    else
                        return;
                }
            }
        }
    }
}

static void
keycode_run()
{
    for (;;) {
        halt();

        key_t key;
        bool  avail;
        while ((avail = kb_getkey(&key)) != false) {
            if (key.ch) {
                tty_printf(
                    TTY_CONSOLE,
                    "Keycode: \033[%c]%02x\033[-] meta=%02x '%c'\n",
                    key.brk == KEYBRK_UP ? 'e' : '2',
                    key.code,
                    key.meta,
                    key.ch);
            }
            else {
                tty_printf(
                    TTY_CONSOLE,
                    "Keycode: \033[%c]%02x\033[-] meta=%02x\n",
                    key.brk == KEYBRK_UP ? 'e' : '2',
                    key.code,
                    key.meta);
            }
            if ((key.brk == KEYBRK_UP) && (key.meta & META_ALT) &&
                (key.code == KEY_TAB)) {
                switch_mode(&mode_shell);
                return;
            }
        }
    }
}

void
kshell()
{
    sort_cmds();

    active_mode->start();
    for (;;)
        active_mode->run();
}
