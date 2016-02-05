//============================================================================
/// @file   io.h
/// @brief  Port I/O routines.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <core.h>

//----------------------------------------------------------------------------
//  @function   io_inb
/// @brief      Retrieve a byte value from an input port.
/// @param[in]  port    Port number (0-65535).
/// @returns    value   Byte value read from the port.
//----------------------------------------------------------------------------
__forceinline uint8_t
io_inb(uint16_t port)
{
    uint8_t value;
    asm volatile ("inb  %[v],   %[p]"
                  : [v] "=a" (value)
                  : [p] "Nd" (port)
                  );
    return value;
}

//----------------------------------------------------------------------------
//  @function   io_outb
/// @brief      Write a byte value to an output port.
/// @param[in]  port    Port number (0-65535).
/// @param[in]  value   Byte value to write to the port.
//----------------------------------------------------------------------------
__forceinline void
io_outb(uint16_t port, uint8_t value)
{
    asm volatile ("outb  %[p],  %[v]"
                  :
                  : [p] "Nd" (port), [v] "a" (value)
                  );
}
