//============================================================================
/// @file       qsort.c
/// @brief      Quicksort algorithm
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#include <libc/stdlib.h>
#include <libc/string.h>

void
qsort(void *base, size_t num, size_t size, sortcmp cmp)
{
    uint8_t pivot[size], tmp[size];     // use C99 VLAs instead of alloca.

    uint8_t *b = (uint8_t *)base;
    for (;;) {
        if (num < 2) {
            return;
        }

        // Use the first element as the pivot.
        memcpy(pivot, b, size);

        // Partition.
        size_t part;
        {
            // Work from outwards in (C.A.R. Hoare version of algorithm).
            uint8_t *i = b - (size * 1);
            uint8_t *j = b + (size * num);

            for (;;) {
                do {
                    i += size;
                } while (cmp(i, pivot) < 0);

                do {
                    j -= size;
                } while (cmp(j, pivot) > 0);

                if (i >= j) {
                    part = (j - b) / size;
                    break;
                }

                // Swap elements i and j.
                memcpy(tmp, i, size);
                memcpy(i, j, size);
                memcpy(j, tmp, size);
            }
        }

        // Recursively sort the left side of the partition.
        qsort(b, part + 1, size, cmp);

        // For the right side of the partition, do tail recursion.
        b   += (part + 1) * size;
        num -= part + 1;
    }
}
