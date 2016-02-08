//============================================================================
/// @file       snprintf.c
/// @brief      Write formatted output to a sized buffer.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#include <libc/stdio.h>

int
snprintf(char *buf, size_t n, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int result = vsnprintf(buf, n, format, args);
    va_end(args);
    return result;
}
