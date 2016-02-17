//============================================================================
/// @file       syscall.c
/// @brief      System call support.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#include <core.h>
#include <kernel/cpu.h>
#include <kernel/interrupt/exception.h>
#include <kernel/interrupt/interrupt.h>
#include <kernel/mem/segments.h>
#include <kernel/syscall/syscall.h>

// Model-specific registers used to set up system calls.
#define MSR_IA32_STAR   0xc0000081
#define MSR_IA32_LSTAR  0xc0000082
#define MSR_IA32_FMASK  0xc0000084

static void
syscall_handle()
{
    // Do nothing for now.
}

void
syscall_init()
{
    // Request the CPU's extended features.
    registers4_t regs;
    cpuid(0x80000001, &regs);

    // Bit 11 of rdx tells us if the SYSCALL/SYSRET instructions are
    // available. If they're not, raise an invalid opcode exception.
    if (!(regs.rdx & (1 << 11))) {
        RAISE_INTERRUPT(EXCEPTION_INVALID_OPCODE);
    }

    // Update the IA32_STAR MSR with the segment selectors that will be used
    // by SYSCALL and SYSRET.
    uint64_t star = rdmsr(MSR_IA32_STAR);
    star &= 0x00000000ffffffff;
    star |= (uint64_t)SEGMENT_SELECTOR_KERNEL_CODE << 32;
    star |= (uint64_t)((SEGMENT_SELECTOR_USER_CODE - 16) | 3) << 48;
    wrmsr(MSR_IA32_STAR, star);

    // Write the address of the system call handler used by SYSCALL.
    wrmsr(MSR_IA32_LSTAR, (uint64_t)syscall_handle);

    // Write the CPU flag mask used during SYSCALL.
    wrmsr(MSR_IA32_FMASK, 0);
}
