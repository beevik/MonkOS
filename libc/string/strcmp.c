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
    int i = 0;
    for (; str1[i] && str2[i]; i++) {
        if (str1[i] == str2[i])
            continue;
        return (int)str1[i] - (int)str2[i];
    }
    return (int)str1[i] - (int)str2[i];
}
