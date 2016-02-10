//============================================================================
/// @file       debug.h
/// @brief      Debug helpers.
//
//  Copyright 2016 Brett Vickers.
//  Use of this source code is governed by a BSD-style license
//  that can be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <core.h>
#include <kernel/cpu.h>

//----------------------------------------------------------------------------
//  @function   debug_dump_registers
/// @brief      Dump the contents of a CPU registers structure to a string
///             buffer.
/// @param[in]  buf     Pointer to the character buffer to hold the resulting
///                     null-terminated string.
/// @param[in]  bufsize Size of the buffer used to hold the string.
/// @param[in]  regs    Pointer to a CPU registers structure.
/// @return     The number of characters that would have been written to the
///             buffer if n had been sufficiently large.
//----------------------------------------------------------------------------
int
debug_dump_registers(char *buf, size_t bufsize, const registers_t *regs);

//----------------------------------------------------------------------------
//  @function   debug_dump_cpuflags
/// @brief      Dump the contents of the CPU flags register.
/// @param[in]  buf     Pointer to the character buffer to hold the resulting
///                     null-terminated string.
/// @param[in]  bufsize Size of the buffer used to hold the string.
/// @param[in]  rflags  The 64-bit rflags register value.
/// @return     The number of characters that would have been written to the
///             buffer if n had been sufficiently large.
//----------------------------------------------------------------------------
int
debug_dump_cpuflags(char *buf, size_t bufsize, uint64_t rflags);

//----------------------------------------------------------------------------
//  @function   debug_dump_memory
/// @brief      Dump the contents of memory.
/// @param[in]  buf     Pointer to the character buffer to hold the resulting
///                     null-terminated string.
/// @param[in]  bufsize Size of the buffer used to hold the string.
/// @param[in]  mem     Pointer to the first byte of memory to dump.
/// @param[in]  memsize Number of memory bytes to dump.
/// @return     The number of characters that would have been written to the
///             buffer if n had been sufficiently large.
//----------------------------------------------------------------------------
int
debug_dump_memory(char *buf, size_t bufsize, const void *mem, size_t memsize);
