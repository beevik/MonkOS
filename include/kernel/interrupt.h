//============================================================================
/// @file       interrupt.h
/// @brief      Interrupt handling operations.
//
//  Copyright 2016 Brett Vickers.
//  Use of this source code is governed by a BSD-style license
//  that can be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <stdint.h>
#include <kernel/compiler.h>
#include <kernel/cpu.h>

//----------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------

// Hardware IRQ values
#define IRQ_TIMER       0
#define IRQ_KEYBOARD    1

// Interrupt vector numbers: CPU exceptions
#define EXCEPTION_DIVBYZERO              0x00
#define EXCEPTION_DEBUG                  0x01
#define EXCEPTION_NMI                    0x02
#define EXCEPTION_BREAKPOINT             0x03
#define EXCEPTION_OVERFLOW               0x04
#define EXCEPTION_BOUNDS                 0x05
#define EXCEPTION_INVALID_OPCODE         0x06
#define EXCEPTION_NO_DEVICE              0x07
#define EXCEPTION_DOUBLE_FAULT           0x08
#define EXCEPTION_COPROCESSOR            0x09
#define EXCEPTION_INVALID_TSS            0x0a
#define EXCEPTION_SEGMENT_NOT_PRESENT    0x0b
#define EXCEPTION_STACK_FAULT            0x0c
#define EXCEPTION_GENERAL_PROTECTION     0x0d
#define EXCEPTION_PAGE_FAULT             0x0e
#define EXCEPTION_FPU                    0x10
#define EXCEPTION_ALIGNMENT              0x11
#define EXCEPTION_MACHINE_CHECK          0x12
#define EXCEPTION_SIMD                   0x13
#define EXCEPTION_VIRTUALIZATION         0x14

// Interrupt vector numbers: hardware IRQ traps
#define TRAP_IRQ_TIMER       0x20
#define TRAP_IRQ_KEYBOARD    0x21

// PIC port constants
#define PIC_PORT_CMD_MASTER     0x20    ///< Command port for master PIC
#define PIC_PORT_CMD_SLAVE      0xa0    ///< Command port for slave PIC
#define PIC_PORT_DATA_MASTER    0x21    ///< Data port for master PIC
#define PIC_PORT_DATA_SLAVE     0xa1    ///< Data port for slave PIC

// PIC commands
#define PIC_CMD_EOI    0x20             ///< End of interrupt

//----------------------------------------------------------------------------
//  @struct interrupt_context
/// @brief      A record describing the CPU state at the time of the
///             interrupt.
//----------------------------------------------------------------------------
typedef struct interrupt_context
{
    registers_t regs;           ///< all general-purpose registers.
    uint64_t    interrupt;      ///< interrupt vector number.
    uint64_t    error;          ///< exception error identifier.
    uint64_t    retaddr;        ///< interrupt return address.
    uint64_t    cs;             ///< code segment.
    uint64_t    rflags;         ///< flags register.
    uint64_t    rsp;            ///< stack pointer.
    uint64_t    ss;             ///< stack segment.
} interrupt_context_t;

//----------------------------------------------------------------------------
//  @function   interrupts_init
/// @brief      Initialize the interrupt service routine table.
/// @details    Clear all interrupt handlers and set up the CPU to properly
///             handle all interrupts.
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
force_inline void
interrupts_enable()
{
    asm volatile ("sti");
}

//----------------------------------------------------------------------------
//  @function   interrupts_disable
/// @brief      Disable interrupts.
//----------------------------------------------------------------------------
force_inline void
interrupts_disable()
{
    asm volatile ("cli");
}

//----------------------------------------------------------------------------
//  @function   halt
/// @brief      Halt the CPU until an interrupt occurs.
//----------------------------------------------------------------------------
force_inline void
halt()
{
    asm volatile ("hlt");
}

//----------------------------------------------------------------------------
//  @function   raise
/// @brief      Raise a software interrupt.
/// @param[in]  vector  The interrupt vector number.
//----------------------------------------------------------------------------
force_inline void
raise(uint8_t vector)
{
    asm volatile ("int %[v]" : : [v] "Nq" (vector));
}
