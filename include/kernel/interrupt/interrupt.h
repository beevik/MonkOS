//============================================================================
/// @file       interrupt.h
/// @brief      Interrupt handling operations.
//
//  Copyright 2016 Brett Vickers.
//  Use of this source code is governed by a BSD-style license
//  that can be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <core.h>
#include <kernel/cpu.h>

//----------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------

// Hardware IRQ values
#define IRQ_TIMER               0
#define IRQ_KEYBOARD            1

// Interrupt vector numbers: hardware IRQ traps
#define TRAP_IRQ_TIMER          0x20
#define TRAP_IRQ_KEYBOARD       0x21

// PIC port constants
#define PIC_PORT_CMD_MASTER     0x20 ///< Command port for master PIC
#define PIC_PORT_CMD_SLAVE      0xa0 ///< Command port for slave PIC
#define PIC_PORT_DATA_MASTER    0x21 ///< Data port for master PIC
#define PIC_PORT_DATA_SLAVE     0xa1 ///< Data port for slave PIC

// PIC commands
#define PIC_CMD_EOI             0x20 ///< End of interrupt

//----------------------------------------------------------------------------
//  @struct interrupt_context
/// @brief      A record describing the CPU state at the time of the
///             interrupt.
//----------------------------------------------------------------------------
struct interrupt_context
{
    registers_t regs;            ///< all general-purpose registers.
    uint64_t    error;           ///< exception error identifier.
    uint64_t    interrupt;       ///< interrupt vector number.
    uint64_t    retaddr;         ///< interrupt return address.
    uint64_t    cs;              ///< code segment.
    uint64_t    rflags;          ///< flags register.
    uint64_t    rsp;             ///< stack pointer.
    uint64_t    ss;              ///< stack segment.
};

typedef struct interrupt_context interrupt_context_t;

//----------------------------------------------------------------------------
//  @function   interrupts_init
/// @brief      Initialize all interrupt tables.
/// @details    Initialize a table of interrupt service routine thunks, one
///             for each of the 256 possible interrupts. Then set up the
///             interrupt descriptor table (IDT) to point to each of the
///             thunks.
///
///             Interrupts should not be enabled until this function has
///             been called.
//----------------------------------------------------------------------------
void
interrupts_init();

//----------------------------------------------------------------------------
//  @typedef    isr_handler
/// @brief      Interrupt service routine called when an interrupt occurs.
/// @param[in]  context     The CPU state at the time of the interrupt.
//----------------------------------------------------------------------------
typedef void (*isr_handler)(const interrupt_context_t *context);

//----------------------------------------------------------------------------
//  @function   isr_set
/// @brief      Set an interrupt service routine for the given interrupt
///             number.
/// @details    Interrupts should be disabled while setting these handlers.
///             To disable an ISR, set its handler to null.
/// @param[in]  interrupt   Interrupt number (0-255).
/// @param[in]  handler     Interrupt service routine handler function.
//----------------------------------------------------------------------------
void
isr_set(int interrupt, isr_handler handler);

//----------------------------------------------------------------------------
//  @function   irq_enable
/// @brief      Tell the PIC to enable a hardware interrupt.
/// @param[in]  irq     IRQ number to enable (0-15).
//----------------------------------------------------------------------------
void
irq_enable(uint8_t irq);

//----------------------------------------------------------------------------
//  @function   irq_disable
/// @brief      Tell the PIC to disable a hardware interrupt.
/// @param[in]  irq     IRQ number to enable (0-15).
//----------------------------------------------------------------------------
void
irq_disable(uint8_t irq);

//----------------------------------------------------------------------------
//  @function   interrupts_enable
/// @brief      Enable interrupts.
//----------------------------------------------------------------------------
__forceinline void
interrupts_enable()
{
    asm volatile ("sti");
}

//----------------------------------------------------------------------------
//  @function   interrupts_disable
/// @brief      Disable interrupts.
//----------------------------------------------------------------------------
__forceinline void
interrupts_disable()
{
    asm volatile ("cli");
}

//----------------------------------------------------------------------------
//  @function   halt
/// @brief      Halt the CPU until an interrupt occurs.
//----------------------------------------------------------------------------
__forceinline void
halt()
{
    asm volatile ("hlt");
}

//----------------------------------------------------------------------------
//  @function   RAISE_INTERRUPT
/// @brief      Issue a software interrupt to the CPU.
/// @param[in]  x       The interrupt to raise. Must be an integer constant
///                     less than 256.
//----------------------------------------------------------------------------
#define RAISE_INTERRUPT(x)    { asm volatile ("int %[n]" ::[n] "N" (x)); }

//----------------------------------------------------------------------------
//  @function   RAISE_BREAKPOINT
/// @brief      Issue a software breakpoint interrupt to the CPU.
//----------------------------------------------------------------------------
#define RAISE_BREAKPOINT()    { asm volatile ("int 3"); }
