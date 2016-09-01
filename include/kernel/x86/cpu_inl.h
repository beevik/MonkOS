//============================================================================
/// @file       cpu_inl.h
/// @brief      x86 CPU-specific function implementations -- inline assembly.
//
//  Copyright 2016 Brett Vickers.
//  Use of this source code is governed by a BSD-style license
//  that can be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <core.h>

#ifndef __NO_INLINE__

__forceinline void
cpuid(uint32_t code, registers4_t *regs)
{
    asm volatile (
        "cpuid"
        : "=a" (regs->rax), "=b" (regs->rbx), "=c" (regs->rcx),
        "=d" (regs->rdx)
        : "0" (code));
}

__forceinline uint64_t
rdmsr(uint32_t id)
{
    uint64_t value;
    asm volatile (
        "rdmsr"
        : "=A" (value)
        : "c" (id));
    return value;
}

__forceinline void
wrmsr(uint32_t id, uint64_t value)
{
    asm volatile (
        "wrmsr"
        :
        : "c" (id), "A" (value));
}

__forceinline uint8_t
io_inb(uint16_t port)
{
    uint8_t value;
    asm volatile (
        "inb  %[v],   %[p]"
        : [v] "=a" (value)
        : [p] "Nd" (port)
        );
    return value;
}

__forceinline void
io_outb(uint16_t port, uint8_t value)
{
    asm volatile (
        "outb  %[p],  %[v]"
        :
        : [p] "Nd" (port), [v] "a" (value)
        );
}

__forceinline uint16_t
io_inw(uint16_t port)
{
    uint16_t value;
    asm volatile (
        "inw  %[v],   %[p]"
        : [v] "=a" (value)
        : [p] "Nd" (port)
        );
    return value;
}

__forceinline void
io_outw(uint16_t port, uint16_t value)
{
    asm volatile (
        "outw  %[p],  %[v]"
        :
        : [p] "Nd" (port), [v] "a" (value)
        );
}

__forceinline uint32_t
io_ind(uint16_t port)
{
    uint32_t value;
    asm volatile (
        "ind  %[v],   %[p]"
        : [v] "=a" (value)
        : [p] "Nd" (port)
        );
    return value;
}

__forceinline void
io_outd(uint16_t port, uint32_t value)
{
    asm volatile (
        "outd  %[p],  %[v]"
        :
        : [p] "Nd" (port), [v] "a" (value)
        );
}

__forceinline void
set_pagetable(uint64_t paddr)
{
    asm volatile (
        "mov    rdi,    %[paddr]\n"
        "mov    cr3,    rdi\n"
        :
        : [paddr] "m" (paddr)
        : "rdi");
}

__forceinline void
invalidate_page(void *vaddr)
{
    asm volatile (
        "invlpg     %[v]\n"
        :
        : [v] "m" (vaddr)
        : "memory" );
}

__forceinline void
enable_interrupts()
{
    asm volatile ("sti");
}

__forceinline void
disable_interrupts()
{
    asm volatile ("cli");
}

__forceinline void
halt()
{
    asm volatile ("hlt");
}

__forceinline void
invalid_opcode()
{
    asm volatile ("int 6");
}

__forceinline void
fatal()
{
    asm volatile ("int 0xff");
}

//
#endif // ifndef __NO_INLINE__
