//============================================================================
/// @file       strcmp.c
/// @brief      Compare one string to another.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#include <core.h>

int
strcmp(const char *str1, const char *str2)
{
    for (; *str1 && *str2; str1++, str2++) {
        if (*str1 == *str2)
            continue;
        return (int)(*str1 - *str2);
    }
    return (int)(*str1 - *str2);
}
