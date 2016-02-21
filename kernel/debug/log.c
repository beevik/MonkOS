//============================================================================
/// @file       log.c
/// @brief      Kernel logging module.
//
//  Copyright 2016 Brett Vickers.
//  Use of this source code is governed by a BSD-style license
//  that can be found in the MonkOS LICENSE file.
//============================================================================

#include <libc/stdio.h>
#include <libc/string.h>
#include <kernel/debug/log.h>

// Record buffer constants
#define RBUFSHIFT      10
#define RBUFSIZE       (1 << RBUFSHIFT) // 1024
#define RBUFMASK       (RBUFSIZE - 1)

// Message buffer constants
#define MBUFSHIFT      16
#define MBUFSIZE       (1 << MBUFSHIFT) // 64KiB
#define MBUFMASK       (MBUFSIZE - 1)

// Callback registrations
#define MAX_CALLBACKS  8

/// A log record represents a single logged event.
typedef struct record
{
    loglevel_t level;
    int        moffset;      ///< Offset of message in context::mbuf
} record_t;

/// Used for registering logging callback functions.
typedef struct callback
{
    loglevel_t   maxlevel;
    log_callback cb;
} callback_t;

/// The log context describes the state of the log buffers. There are two
/// buffers: the record buffer and the message buffer. The record buffer is a
/// circular queue that contains records describing the most recent 1024 log
/// messages. The message buffer contains the text messages associated with
/// each of the records in the log buffer.
struct context
{
    // Record buffer
    record_t rbuf[RBUFSIZE]; ///< Circular record buffer
    int      rhead;          ///< index of oldest record in rbuf
    int      rtail;          ///< rbuf index of next record to write
    int      rbufsz;         ///< number of records in rbuf

    // Message buffer
    char mbuf[MBUFSIZE]; ///< Circular message text buffer
    int  mhead;          ///< index of oldest char in mbuf
    int  mtail;          ///< mbuf index of next char to write
    int  mbufsz;         ///< the number of characters in mbuf

    // Callback registrations
    callback_t callbacks[MAX_CALLBACKS];
    int        callbacks_size; ///< number of registrations
};

static struct context lc;

static void
evict_record()
{
    lc.rhead = (lc.rhead + 1) & RBUFMASK;
    lc.rbufsz--;
}

static void
evict_msg(int chars)
{
    // Evict the requested number of characters from the mbuf. Evict
    // associated records when the end of a message is reached.
    for (int i = 0; i < chars && lc.mbufsz > 0; i++) {
        if (lc.mbuf[lc.mhead] == 0)
            evict_record();
        lc.mhead = (lc.mhead + 1) & MBUFMASK;
        lc.mbufsz--;
    }

    // Keep evicting characters from the message buffer until a null
    // terminator is reached.
    while (lc.mbufsz > 0) {
        char ch = lc.mbuf[lc.mhead];
        lc.mhead = (lc.mhead + 1) & MBUFMASK;
        lc.mbufsz--;
        if (ch == 0) {
            evict_record();
            break;
        }
    }
}

static void
evict_oldest_msg()
{
    while (lc.mbufsz > 0) {
        char ch = lc.mbuf[lc.mhead];
        lc.mhead = (lc.mhead + 1) & MBUFMASK;
        lc.mbufsz--;
        if (ch == 0)
            break;
    }
}

static int
add_msg(const char *str)
{
    int offset = lc.mtail;

    // Evict one or more log records if we're going to exceed the buffer size.
    int schars = strlen(str) + 1;
    if (lc.mbufsz + schars > MBUFSIZE)
        evict_msg(lc.mbufsz + schars - MBUFSIZE);

    // Copy the string into the buffer, handling wrap-around.
    if (lc.mtail + schars > MBUFSIZE) {
        int split = MBUFSIZE - lc.mtail;
        memcpy(lc.mbuf + lc.mtail, str, split);
        memcpy(lc.mbuf, str + split, schars - split);
        lc.mtail = schars - split;
    }
    else {
        memcpy(lc.mbuf + lc.mtail, str, schars);
        lc.mtail += schars;
    }

    lc.mbufsz += schars;

    return offset;
}

static void
add_record(int offset, loglevel_t level, const char *str)
{
    // If the record buffer is full, evict the oldest record and its
    // associated message text.
    if (lc.rbufsz == RBUFSIZE) {
        evict_record();
        evict_oldest_msg();
    }

    // Add a new record on the tail of the circular record queue.
    record_t *r = lc.rbuf + lc.rtail;
    r->moffset = offset;
    r->level   = level;

    lc.rtail = (lc.rtail + 1) & RBUFMASK;
    lc.rbufsz++;

    // Issue registered log callbacks.
    for (int i = 0; i < lc.callbacks_size; i++) {
        const callback_t *callback = &lc.callbacks[i];
        if (level <= callback->maxlevel)
            callback->cb(level, str);
    }
}

void
log_addcallback(loglevel_t maxlevel, log_callback cb)
{
    if (lc.callbacks_size == MAX_CALLBACKS)
        return;

    callback_t *record = &lc.callbacks[lc.callbacks_size++];
    record->maxlevel = maxlevel;
    record->cb       = cb;
}

void
log_removecallback(log_callback cb)
{
    for (int i = 0; i < lc.callbacks_size; i++) {
        if (lc.callbacks[i].cb == cb) {
            memmove(lc.callbacks + i, lc.callbacks + i + 1,
                    sizeof(callback_t) * (lc.callbacks_size - (i + 1)));
            return;
        }
    }
}

void
log(loglevel_t level, const char *str)
{
    int offset = add_msg(str);
    add_record(offset, level, str);
}

void
logf(loglevel_t level, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    logvf(level, format, args);
    va_end(args);
}

void
logvf(loglevel_t level, const char *format, va_list args)
{
    char str[1024];
    vsnprintf(str, arrsize(str), format, args);
    int offset = add_msg(str);
    add_record(offset, level, str);
}
