//============================================================================
/// @file       cpu.h
/// @brief      x86 CPU-specific function implementations.
//
//  Copyright 2016 Brett Vickers.
//  Use of this source code is governed by a BSD-style license
//  that can be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <core.h>

// CPU EFLAGS register values
#define CPU_EFLAGS_CARRY       (1 << 0)
#define CPU_EFLAGS_PARITY      (1 << 2)
#define CPU_EFLAGS_ADJUST      (1 << 4)
#define CPU_EFLAGS_ZERO        (1 << 6)
#define CPU_EFLAGS_SIGN        (1 << 7)
#define CPU_EFLAGS_TRAP        (1 << 8)
#define CPU_EFLAGS_INTERRUPT   (1 << 9)
#define CPU_EFLAGS_DIRECTION   (1 << 10)
#define CPU_EFLAGS_OVERFLOW    (1 << 11)
#define CPU_EFLAGS_IOPL1       (1 << 12)
#define CPU_EFLAGS_IOPL0       (1 << 13)
#define CPU_EFLAGS_NESTED      (1 << 14)
#define CPU_EFLAGS_RESUME      (1 << 16)
#define CPU_EFLAGS_V8086       (1 << 17)
#define CPU_EFLAGS_ALIGNCHECK  (1 << 18)
#define CPU_EFLAGS_VINTERRUPT  (1 << 19)
#define CPU_EFLAGS_VPENDING    (1 << 20)
#define CPU_EFLAGS_CPUID       (1 << 21)

//----------------------------------------------------------------------------
//  @struct     registers_t
/// @brief      A record describing all 64-bit general-purpose registers.
//----------------------------------------------------------------------------
typedef struct registers
{
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
} registers_t;

//----------------------------------------------------------------------------
//  @struct     registers4_t
/// @brief      A record describing the first 4 general-purpose registers.
//----------------------------------------------------------------------------
typedef struct registers4
{
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
} registers4_t;


#ifdef __NO_INLINE__

//----------------------------------------------------------------------------
//  @function   cpuid
/// @brief      Return the results of the CPUID instruction.
/// @param[in]  code    The cpuid group code.
/// @param[out] regs    The contents of registers rax, rbx, rcx, and rdx.
//----------------------------------------------------------------------------
void
cpuid(uint32_t code, registers4_t *regs);

//----------------------------------------------------------------------------
//  @function   rdmsr
/// @brief      Read the model-specific register and return the result.
/// @param[in]  id      The register id to read.
/// @returns    The contents of the requested MSR.
//----------------------------------------------------------------------------
uint64_t
rdmsr(uint32_t id);

//----------------------------------------------------------------------------
//  @function   wrmsr
/// @brief      Write to the model-specific register.
/// @param[in]  id      The register id to write.
/// @param[in]  value   The value to write.
//----------------------------------------------------------------------------
void
wrmsr(uint32_t id, uint64_t value);

//----------------------------------------------------------------------------
//  @function   io_inb
/// @brief      Retrieve a byte value from an input port.
/// @param[in]  port    Port number (0-65535).
/// @returns    value   Byte value read from the port.
//----------------------------------------------------------------------------
uint8_t
io_inb(uint16_t port);

//----------------------------------------------------------------------------
//  @function   io_outb
/// @brief      Write a byte value to an output port.
/// @param[in]  port    Port number (0-65535).
/// @param[in]  value   Byte value to write to the port.
//----------------------------------------------------------------------------
void
io_outb(uint16_t port, uint8_t value);

//----------------------------------------------------------------------------
//  @function   io_inw
/// @brief      Retrieve a 16-bit word value from an input port.
/// @param[in]  port    Port number (0-65535).
/// @returns    value   Word value read from the port.
//----------------------------------------------------------------------------
uint16_t
io_inw(uint16_t port);

//----------------------------------------------------------------------------
//  @function   io_outw
/// @brief      Write a 16-bit word value to an output port.
/// @param[in]  port    Port number (0-65535).
/// @param[in]  value   Word value to write to the port.
//----------------------------------------------------------------------------
void
io_outw(uint16_t port, uint16_t value);

//----------------------------------------------------------------------------
//  @function   io_ind
/// @brief      Retrieve a 32-bit dword value from an input port.
/// @param[in]  port    Port number (0-65535).
/// @returns    value   Dword value read from the port.
//----------------------------------------------------------------------------
uint32_t
io_ind(uint16_t port);

//----------------------------------------------------------------------------
//  @function   io_outd
/// @brief      Write a 32-bit dword value to an output port.
/// @param[in]  port    Port number (0-65535).
/// @param[in]  value   Dword value to write to the port.
//----------------------------------------------------------------------------
void
io_outd(uint16_t port, uint32_t value);

//----------------------------------------------------------------------------
//  @function   set_pagetable
/// @brief      Update the CPU's page table register.
/// @param[in]  paddr   The physical address containing the new pagetable.
//----------------------------------------------------------------------------
void
set_pagetable(uint64_t paddr);

//----------------------------------------------------------------------------
//  @function   enable_interrupts
/// @brief      Enable interrupts.
//----------------------------------------------------------------------------
void
enable_interrupts();

//----------------------------------------------------------------------------
//  @function   disable_interrupts
/// @brief      Disable interrupts.
//----------------------------------------------------------------------------
void
disable_interrupts();

//----------------------------------------------------------------------------
//  @function   halt
/// @brief      Halt the CPU until an interrupt occurs.
//----------------------------------------------------------------------------
void
halt();

//----------------------------------------------------------------------------
//  @function   invalid_opcode
/// @brief      Raise an invalid opcode exception.
//----------------------------------------------------------------------------
void
invalid_opcode();

//----------------------------------------------------------------------------
//  @function   fatal
/// @brief      Raise a fatal interrupt that hangs the system.
//----------------------------------------------------------------------------
void
fatal();

// Use inline assembly versions if __NO_INLINE__ is not defined
#else
#    include "cpu_inl.h"
#endif
