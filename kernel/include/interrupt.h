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

// PIC port constants
#define PIC_PORT_CMD_MASTER     0x20    ///< Command port for master PIC
#define PIC_PORT_CMD_SLAVE      0xa0    ///< Command port for slave PIC
#define PIC_PORT_DATA_MASTER    0x21    ///< Data port for master PIC
#define PIC_PORT_DATA_SLAVE     0xa1    ///< Data port for slave PIC

// PIC commands
#define PIC_CMD_EOI             0x20    ///< End of interrupt

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
/// @param[in]  interrupt   Interrupt number (0-255).
/// @param[in]  error       Error code (interrupt-specific meaning).
//----------------------------------------------------------------------------
typedef void (*isr_handler)(uint8_t interrupt, uint64_t error);

//----------------------------------------------------------------------------
//  @function   interrupts_enable
/// @brief      Enable all defined (maskable) interrupt service routines.
/// @details    Do not call this from within an ISR, as the original
///             interrupt flag will be restored as soon as the ISR returns.
//----------------------------------------------------------------------------
void
interrupts_enable();

//----------------------------------------------------------------------------
//  @function   interrupts_disable
/// @brief      Disable all defined (maskable) interrupt service routines.
/// @details    Do not call this from within an ISR, as the original
///             interrupt flag will be restored as soon as the ISR returns.
//----------------------------------------------------------------------------
void
interrupts_disable();

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
//  @function   halt
/// @brief        Halt the computer until an interrupt arrives.
//----------------------------------------------------------------------------
void
halt();
