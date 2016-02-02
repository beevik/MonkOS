//============================================================================
/// @file       timer.c
/// @brief      Programmable interval timer (8253/8254) controller.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <kernel/asm/io.h>
#include <kernel/interrupt.h>
#include <kernel/timer.h>

// 8253 timer ports
#define TIMER_PORT_DATA_CH0    0x40     ///< Channel 0 data port.
#define TIMER_PORT_DATA_CH1    0x41     ///< Channel 1 data port.
#define TIMER_PORT_DATA_CH2    0x42     ///< Channel 2 data port.
#define TIMER_PORT_CMD         0x43     ///< Timer command port.

// Frequency bounds
#define MIN_FREQUENCY          19
#define MAX_FREQUENCY          1193181

//----------------------------------------------------------------------------
//  @function   isr_timer
/// @brief      Interrupt service routine for timer IRQ 0.
/// @param[in]  interrupt   Interrupt number (unused).
/// @param[in]  error       Interrupt error code (unused).
//----------------------------------------------------------------------------
static void
isr_timer(uint8_t interrupt, uint64_t error)
{
    (void)interrupt;
    (void)error;

    // Do nothing for now.

    // Send the end-of-interrupt signal.
    io_outb(PIC_PORT_CMD_MASTER, PIC_CMD_EOI);
}

//----------------------------------------------------------------------------
//  @function   timer_init
//  @brief      Initialize the timer controller so that it interrupts the
//              kernel at the requested frequency.
//  @param[in]  frequency   The interrupt frequency in Hz. Clamped to the
//                          range [19:1193181].
//----------------------------------------------------------------------------
void
timer_init(uint32_t frequency)
{
    // Clamp frequency to allowable range.
    if (frequency < MIN_FREQUENCY)
        frequency = MIN_FREQUENCY;
    else if (frequency > MAX_FREQUENCY)
        frequency = MAX_FREQUENCY;

    // Compute the clock count value.
    uint16_t count = (uint16_t)(MAX_FREQUENCY / frequency);

    // Channel=0, AccessMode=lo/hi, OperatingMode=rate-generator
    io_outb(TIMER_PORT_CMD, 0x34);

    // Output the lo/hi count value
    io_outb(TIMER_PORT_DATA_CH0, (uint8_t)count);
    io_outb(TIMER_PORT_DATA_CH0, (uint8_t)(count >> 8));

    // Assign the interrupt service routine.
    isr_set(0x20, isr_timer);

    // Enable the timer interrupt (IRQ0).
    irq_enable(0);
}

//----------------------------------------------------------------------------
//  @function   timer_enable
//  @brief      Enable timer interrupts.
//----------------------------------------------------------------------------
void
timer_enable()
{
    // Enable the timer interrupt (IRQ0).
    irq_enable(0);
}

//----------------------------------------------------------------------------
//  @function   timer_disable
//  @brief      Disable timer interrupts.
//----------------------------------------------------------------------------
void
timer_disable()
{
    // Disable the timer interrupt (IRQ0).
    irq_disable(0);
}
