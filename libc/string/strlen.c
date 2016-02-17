//============================================================================
/// @file       strlen.c
/// @brief      Returns the length of the a string.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#include <core.h>

size_t
strlen(const char *str)
{
    size_t len = 0;
    for (; str[len]; ++len) {
        // do nothing
    }
    return len;
}
