//============================================================================
/// @file       vsnprintf.c
/// @brief      Write formatted data from a variable argument list to a sized
//              buffer.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//
// This file is a heavily-modified version of the klibc vsnprintf function,
// found at:
//
//    https://github.com/brainflux/klibc/tree/master/usr/klibc
//
// The relevant portion of the klibc LICENSE reads as follows:
//
// * Permission is hereby granted, free of charge, to any person obtaining
// * a copy of this software and associated documentation files (the
// * "Software"), to deal in the Software without restriction, including
// * without limitation the rights to use, copy, modify, merge, publish,
// * distribute, sublicense, and/or sell copies of the Software, and to
// * permit persons to whom the Software is furnished to do so, subject to
// * the following conditions:
// *
// * Any copyright notice(s) and this permission notice shall be
// * included in all copies or substantial portions of the Software.
// *
// * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//============================================================================

#include <libc/stdio.h>
#include <libc/string.h>

// Parser state
enum state
{
    STATE_CHARS,
    STATE_FLAGS,                 // -,+, ,#,0
    STATE_WIDTH,                 // #
    STATE_PRECISION,             // .#
    STATE_DATALEN,               // h,hh,l,ll,j,z,t
    STATE_DATATYPE,              // d,i,u,o,x,X,c,s,p,n
};

// Parse flags
#define FLAG_ZERO    (1 << 0)
#define FLAG_MINUS   (1 << 1)
#define FLAG_PLUS    (1 << 2)
#define FLAG_SIGNED  (1 << 3)
#define FLAG_UPPER   (1 << 4)
#define FLAG_SPACE   (1 << 5)
#define FLAG_HASH    (1 << 6)

// Data length modifiers
enum datalen
{
    DATALEN_CHAR,
    DATALEN_SHORT,
    DATALEN_INT,
    DATALEN_LONG,
    DATALEN_LONGLONG,
    DATALEN_INTMAX_T,
    DATALEN_SIZE_T,
    DATALEN_PTRDIFF_T,
};

// Parser state data
struct parse
{
    char        *bufptr;
    const char  *bufterm;
    enum state   state;
    uint32_t     flags;          // FLAG_*
    int          width;
    int          precision;
    enum datalen datalen;
    int          base;
};

static const char digits_lower[] = "0123456789abcdef";
static const char digits_upper[] = "0123456789ABCDEF";

static __forceinline bool
isdigit(char ch)
{
    return(ch >= '0' && ch <= '9');
}

static __forceinline void
addchar(struct parse *parse, char ch)
{
    if (parse->bufptr < parse->bufterm) {
        *parse->bufptr = ch;
    }
    ++parse->bufptr;
}

static void
addstring(struct parse *parse, const char *s)
{
    // Null string should display "(null)".
    if (s == 0) {
        s = "(null)";
    }

    int slen = strlen(s);

    // Truncate string at precision length.
    if ((parse->precision > -1) && (parse->precision < slen)) {
        slen = parse->precision;
    }

    // Pad on left with zeroes or spaces.
    if ((parse->width > slen) && !(parse->flags & FLAG_MINUS)) {
        char pad = (parse->flags & FLAG_ZERO) ? '0' : ' ';
        while (parse->width > slen) {
            addchar(parse, pad);
            --parse->width;
        }
    }

    // Add string.
    for (int i = slen; i; i--) {
        addchar(parse, *s++);
    }

    // Pad on right with spaces.
    if ((parse->width > slen) && (parse->flags & FLAG_MINUS)) {
        while (parse->width > slen) {
            addchar(parse, ' ');
            --parse->width;
        }
    }
}

static void
addint(struct parse *parse, uintmax_t value)
{
    bool minus = false;

    // Check for preceding minus sign.
    if ((parse->flags & FLAG_SIGNED) && ((intmax_t)value < 0)) {
        minus = true;
        value = (uintmax_t)(-(intmax_t)value);
    }

    // Count the number of digits.
    int       ndigits = 0;
    uintmax_t tmpval  = value;
    while (tmpval) {
        tmpval /= parse->base;
        ndigits++;
    }

    // In octal mode, make space for a preceding zero.
    if ((parse->flags & FLAG_HASH) && (parse->base == 8)) {
        if (parse->precision < ndigits + 1) {
            parse->precision = ndigits + 1;
        }
    }

    // Need enough digits to account for precision setting.
    if (ndigits < parse->precision) {
        ndigits = parse->precision;
    }

    // Need 1 digit if value is zero.
    else if (value == 0) {
        ndigits = 1;
    }

    // Count the number of additional non-digit prefix characters.
    int nchars = ndigits;
    if (minus || (parse->flags & (FLAG_PLUS | FLAG_SPACE))) {
        ++nchars;
    }
    if ((parse->flags & FLAG_HASH) && (parse->base == 16)) {
        nchars += 2;
    }

    // Pad to width with spaces, if requested.
    if (!(parse->flags & (FLAG_MINUS | FLAG_ZERO)) &&
        (parse->width > nchars)) {
        while (parse->width > nchars) {
            addchar(parse, ' ');
            parse->width--;
        }
    }

    // Add prefix symbol.
    if (minus) {
        addchar(parse, '-');
    }
    else if (parse->flags & FLAG_PLUS) {
        addchar(parse, '+');
    }
    else if (parse->flags & FLAG_SPACE) {
        addchar(parse, ' ');
    }

    // Add "0x" prefix in hexadecimal mode.
    if ((parse->flags & FLAG_HASH) && (parse->base == 16)) {
        addchar(parse, '0');
        addchar(parse, (parse->flags & FLAG_UPPER) ? 'X' : 'x');
    }

    // Prepend zeroes if requested and needed.
    if (((parse->flags & (FLAG_MINUS | FLAG_ZERO)) == FLAG_ZERO) &&
        (parse->width > ndigits)) {
        while (parse->width > nchars) {
            addchar(parse, '0');
            parse->width--;
        }
    }

    // Advance the buffer pointer (possibly beyond the end of the buffer
    // terminator).
    parse->bufptr += ndigits;

    // Select lower-case or upper-case hexadecimal digits.
    const char *digits = (parse->flags & FLAG_UPPER)
                         ? digits_upper : digits_lower;

    // Generate the digits backwards.
    char *ptr = parse->bufptr;
    while (ndigits > 0) {
        --ptr;
        --ndigits;
        if (ptr < parse->bufterm) {
            *ptr = digits[value % parse->base];
        }
        value /= parse->base;
    }

    // Pad width on right, if requested.
    if (parse->flags & FLAG_MINUS) {
        while (parse->width > nchars) {
            addchar(parse, ' ');
            --parse->width;
        }
    }
}

int
vsnprintf(char *buf, size_t n, const char *format, va_list args)
{
    const char *fptr = format;

    struct parse parse =
    {
        .bufptr    = buf,
        .bufterm   = buf + n,
        .state     = STATE_CHARS,
        .flags     = 0,
        .width     = 0,
        .precision = -1,
        .datalen   = DATALEN_INT,
        .base      = 10,
    };

    char ch;
    while ((ch = *fptr++) != 0) {

        // Standard characters (i.e., not format specifiers)
        if (parse.state == STATE_CHARS) {
            if (ch == '%') {
                parse.state     = STATE_FLAGS;
                parse.flags     = 0;
                parse.width     = 0;
                parse.precision = -1;
                parse.datalen   = DATALEN_INT;
                parse.base      = 10;
            }
            else {
                addchar(&parse, ch);
            }
        }

        // Format specifier: flags
        else if (parse.state == STATE_FLAGS) {
            switch (ch)
            {
                case '0':
                    parse.flags |= FLAG_ZERO;
                    break;

                case '-':
                    parse.flags |= FLAG_MINUS;
                    break;

                case '+':
                    parse.flags |= FLAG_PLUS;
                    break;

                case ' ':
                    parse.flags |= FLAG_SPACE;
                    break;

                case '#':
                    parse.flags |= FLAG_HASH;
                    break;

                default:
                    parse.state = STATE_WIDTH;
                    --fptr;      // back up and try again
                    break;
            }
        }

        // Format specifier: width
        else if (parse.state == STATE_WIDTH) {
            if (isdigit(ch)) {
                parse.width = (parse.width * 10) + (ch - '0');
            }
            else if (ch == '*') {
                parse.width = va_arg(args, int);
                if (parse.width < 0) {
                    parse.width  = -parse.width;
                    parse.flags |= FLAG_MINUS;
                }
            }
            else if (ch == '.') {
                parse.precision = 0;
                parse.state     = STATE_PRECISION;
            }
            else {
                parse.state = STATE_DATALEN;
                --fptr;          // back up and try again
            }
        }

        // Format specifier: .precision
        else if (parse.state == STATE_PRECISION) {
            if (isdigit(ch)) {
                parse.precision = (parse.precision * 10) + (ch - '0');
            }
            else if (ch == '*') {
                parse.precision = va_arg(args, int);
                if (parse.precision < 0) {
                    parse.precision = -1;
                }
            }
            else {
                parse.state = STATE_DATALEN;
                --fptr;          // back up and try again
            }
        }

        // Format specifier: data length
        else if (parse.state == STATE_DATALEN) {
            switch (ch)
            {
                case 'l':
                    parse.datalen = DATALEN_LONG;
                    if (*fptr == 'l') {
                        fptr++;
                        parse.datalen = DATALEN_LONGLONG;
                    }
                    break;

                case 'h':
                    parse.datalen = DATALEN_SHORT;
                    if (*fptr == 'h') {
                        fptr++;
                        parse.datalen = DATALEN_CHAR;
                    }
                    break;

                case 'j':
                    parse.datalen = DATALEN_INTMAX_T;
                    break;

                case 'z':
                    parse.datalen = DATALEN_SIZE_T;
                    break;

                case 't':
                    parse.datalen = DATALEN_PTRDIFF_T;
                    break;

                default:
                    --fptr;      // back up and try again
                    break;
            }
            parse.state = STATE_DATATYPE;
        }

        // Format specifier: data type
        else if (parse.state == STATE_DATATYPE) {

            uintmax_t value = (uintmax_t)0;

            switch (ch)
            {
                case 's':
                    addstring(&parse, va_arg(args, const char *));
                    break;

                case 'c':
                    {
                        char cbuf[2] = { (char)va_arg(args, int), 0 };
                        addstring(&parse, cbuf);
                    }
                    break;

                case 'd':
                case 'i':
                    parse.flags |= FLAG_SIGNED;
                    parse.base   = 10;
                    goto add_signed;

                case 'u':
                    parse.base = 10;
                    goto add_unsigned;

                case 'X':
                    parse.flags |= FLAG_UPPER;
                // fallthru

                case 'x':
                    parse.base = 16;
                    goto add_unsigned;

                case 'o':
                    parse.base = 8;
                    goto add_unsigned;

                case 'P':
                    parse.flags |= FLAG_UPPER;
                // fallthru

                case 'p':
                    parse.base      = 16;
                    parse.flags    |= FLAG_HASH;
                    parse.precision = 2 * sizeof(void *);
                    parse.datalen   = DATALEN_PTRDIFF_T;
                    goto add_unsigned;

                case 'n':
                    *(int *)va_arg(args, int *) = (int)(parse.bufptr - buf);
                    break;

                default:
                    addchar(&parse, ch);
                    break;

                add_signed:
                    switch (parse.datalen)
                    {
                        case DATALEN_CHAR:
                            value = (uintmax_t)(intmax_t)(signed char)
                                    va_arg(args, signed int);
                            break;

                        case DATALEN_SHORT:
                            value = (uintmax_t)(intmax_t)(signed short)
                                    va_arg(args, signed int);
                            break;

                        case DATALEN_INT:
                            value = (uintmax_t)(intmax_t)(signed int)
                                    va_arg(args, signed int);
                            break;

                        case DATALEN_LONG:
                            value = (uintmax_t)(intmax_t)(signed long)
                                    va_arg(args, signed long);
                            break;

                        case DATALEN_LONGLONG:
                            value = (uintmax_t)(intmax_t)(signed long long)
                                    va_arg(args, signed long long);
                            break;

                        case DATALEN_INTMAX_T:
                            value = (uintmax_t)(intmax_t)
                                    va_arg(args, intmax_t);
                            break;

                        case DATALEN_SIZE_T:
                            value = (uintmax_t)(size_t)
                                    va_arg(args, size_t);
                            break;

                        case DATALEN_PTRDIFF_T:
                            value = (uintmax_t)(ptrdiff_t)
                                    va_arg(args, ptrdiff_t);
                            break;

                    }
                    addint(&parse, value);
                    break;

                add_unsigned:
                    switch (parse.datalen)
                    {
                        case DATALEN_CHAR:
                            value = (uintmax_t)(uint8_t)
                                    va_arg(args, unsigned int);
                            break;

                        case DATALEN_SHORT:
                            value = (uintmax_t)(uint16_t)
                                    va_arg(args, unsigned int);
                            break;

                        case DATALEN_INT:
                            value = (uintmax_t)(unsigned int)
                                    va_arg(args, unsigned int);
                            break;

                        case DATALEN_LONG:
                            value = (uintmax_t)(unsigned long)
                                    va_arg(args, unsigned long);
                            break;

                        case DATALEN_LONGLONG:
                            value = (uintmax_t)(unsigned long long)
                                    va_arg(args, unsigned long long);
                            break;

                        case DATALEN_INTMAX_T:
                            value = (uintmax_t)
                                    va_arg(args, uintmax_t);
                            break;

                        case DATALEN_SIZE_T:
                            value = (uintmax_t)(size_t)
                                    va_arg(args, size_t);
                            break;

                        case DATALEN_PTRDIFF_T:
                            value = (uintmax_t)(ptrdiff_t)
                                    va_arg(args, ptrdiff_t);
                            break;
                    }
                    addint(&parse, value);
                    break;
            }

            parse.state = STATE_CHARS;
        }

    }

    // Add a null terminator.
    if (parse.bufptr < parse.bufterm) {
        *parse.bufptr = 0;
    }
    else if (n > 0) {
        parse.bufptr[-1] = 0;
    }

    // Return the number of characters processed.
    return (int)(parse.bufptr - buf);
}
