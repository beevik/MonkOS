//============================================================================
/// @file       exception.h
/// @brief      CPU exceptions.
//
//  Copyright 2016 Brett Vickers.
//  Use of this source code is governed by a BSD-style license
//  that can be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <stdint.h>
#include <kernel/compiler.h>

// CPU exception constants
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


//----------------------------------------------------------------------------
//  @function   exceptions_init
/// @brief      Initialize all exception handling routines.
//----------------------------------------------------------------------------
void
exceptions_init();
