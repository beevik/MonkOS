//============================================================================
/// @file   kernel/asm/io.h
/// @brief  Inline assembly for port I/O routines.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <stdint.h>
#include <kernel/asm/asm.h>

//----------------------------------------------------------------------------
//  @function   io_inb
/// @brief      Retrieve a byte value from an input port.
/// @param[in]  port    Port number (0-65535).
/// @returns    value   Byte value read from the port.
//----------------------------------------------------------------------------
force_inline uint8_t
io_inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ("in %0, %1"
                  : "=a" (ret)
                  : "Nd" (port));
    return ret;
}

//----------------------------------------------------------------------------
//  @function   io_outb
/// @brief      Write a byte value to an output port.
/// @param[in]  port    Port number (0-65535).
/// @param[in]  value   Byte value to write to the port.
//----------------------------------------------------------------------------
force_inline void
io_outb(uint16_t port, uint8_t value)
{
    asm volatile ("out %0, %1"
                  :
                  : "Nd" (port), "a" (value));
}
