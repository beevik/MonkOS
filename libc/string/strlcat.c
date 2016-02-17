//============================================================================
/// @file       strlcat.c
/// @brief      Appends one string to another.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#include <core.h>

size_t
strlcat(char *dst, const char *src, size_t dstsize)
{
    char       *d  = dst;
    const char *dt = dst + dstsize - 1;
    while (d < dt && *d) {
        d++;
    }

    const char *s = src;
    while (d < dt && *s) {
        *d++ = *s++;
    }

    if (dstsize > 0) {
        *d = 0;
    }

    return (size_t)(d - dst);
}
