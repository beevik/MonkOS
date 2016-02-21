//============================================================================
/// @file       acpi.h
/// @brief      Advanced configuration and power interface (ACPI) tables.
//
//  Copyright 2016 Brett Vickers.
//  Use of this source code is governed by a BSD-style license
//  that can be found in the MonkOS LICENSE file.
//============================================================================

#pragma once

#include <core.h>

//----------------------------------------------------------------------------
//  @struct     acpi_hdr
/// @brief      Header attached to the front of all ACPI tables.
//----------------------------------------------------------------------------
struct acpi_hdr
{
    char     signature[4];     ///< Contains 4-letter table identifier
    uint32_t length;           ///< Length of this table including header
    uint8_t  revision;         ///< Revision number, should be 1
    uint8_t  checksum;         ///< Covers entire table
    char     oemid[6];         ///< Supplied by the OEM
    char     oemtableid[8];    ///< Supplied by the OEM
    uint32_t oemrevision;      ///< Supplied by the OEM
    char     creatorid[4];     ///< Vendor id
    uint32_t creator_revision; ///< Revision of this utility
} PACKSTRUCT;

//----------------------------------------------------------------------------
//  @struct     acpi_fadt
/// @brief      Fixed ACPI Description Table (FADT)
//----------------------------------------------------------------------------
struct acpi_fadt
{
    struct acpi_hdr hdr;

    uint32_t firmware_ctl;  ///< Pointer to FACS firmware control block
    uint32_t ptr_dsdt;      ///< Pointer to DSDT block
    uint8_t  reserved1;     ///< Not used
    uint8_t  pm_profile;    ///< Preferred power management profile
    uint16_t sci_interrupt; ///< SCI interrupt vector
    uint32_t smi_cmdport;   ///< SMI command port
    uint8_t  acpi_enable;   ///< SMI command to disable SMI ownership
    uint8_t  acpi_disable;  ///< SMI command to re-enable SMI ownership
    uint8_t  s4bios_req;    ///< SMI command to enter S4BIOS state
    uint8_t  pstate_ctl;    ///< SMI command to assume perf state ctl
    uint32_t pm1a_evtblock; ///< Port of PM1a event register block
    uint32_t pm1b_evtblock; ///< Port of PM1b event register block
    uint32_t pm1a_ctlblock; ///< Port of PM1a ctl register block
    uint32_t pm1b_ctlblock; ///< Port of PM1b ctl register block
    uint32_t pm2_ctlblock;  ///< Port of PM2 ctl register block
    uint32_t pmt_ctlblock;  ///< Port of PM timer ctl register block
    uint32_t gpe0_block;    ///< Port of general-purpose event 0 reg block
    uint32_t gpe1_block;    ///< Port of general-purpose event 0 reg block
    uint8_t  pm1_evt_len;   ///< Bytes decoded by pm1*_evtblock
    uint8_t  pm1_ctl_len;   ///< Bytes decoded by pm1*_ctlblock
    uint8_t  pm2_ctl_len;   ///< Bytes decoded by pm2_ctlblock
    uint8_t  pmt_ctl_len;   ///< Bytes decoded by pmt_ctlblock
    uint8_t  gpe0_len;      ///< Bytes decoded by gpe0_block
    uint8_t  gpe1_len;      ///< Bytes decoded by gpe1_block
    uint8_t  gpe1_base;     ///< Offset where gpe1 events start
    uint8_t  cstate_ctl;    ///< SMI command for C state notifications
    uint16_t latency_c2;    ///< Worst-case us latency to enter C2 state
    uint16_t latency_c3;    ///< Worst-case us latency to enter C3 state
    uint16_t flush_size;    ///< Cache reads to flush dirty cache
    uint16_t flush_stride;  ///< Cache width (flush stride)
    uint8_t  duty_offset;   ///< Index of P_CNT reg duty cycle setting
    uint8_t  duty_width;    ///< Width of P_CNT reg duty cycle setting
    uint8_t  alarm_day;     ///< RTC RAM index day-of-month alarm: day
    uint8_t  alarm_month;   ///< RTC RAM index day-of-month alarm: month
    uint8_t  century;       ///< RTC RAM index of century
    uint16_t boot_arch;     ///< Boot architecture flags (ACPI 2.0+)
    uint8_t  reserved2;     ///< Not used
    uint32_t flags;         ///< Fixed feature flags
} PACKSTRUCT;

//----------------------------------------------------------------------------
//  @struct     acpi_madt
/// @brief      Multiple APIC description table (MADT).
//----------------------------------------------------------------------------
struct acpi_madt
{
    struct acpi_hdr hdr;

    uint32_t ptr_local_apic;   ///< Local APIC address
    uint32_t flags;            ///< APIC flags
} PACKSTRUCT;

//----------------------------------------------------------------------------
//  @enum       acpi_madt_typpe
/// @brief      MADT entry types.
//----------------------------------------------------------------------------
enum acpi_madt_type
{
    ACPI_MADT_LOCAL_APIC   = 0,  ///< Processor Local APIC
    ACPI_MADT_IO_APIC      = 1,  ///< I/O APIC
    ACPI_MADT_ISO          = 2,  ///< Interrupt Source Override
    ACPI_MADT_NMIS         = 3,  ///< NMI Source
    ACPI_MADT_LOCAL_NMI    = 4,  ///< Local APIC NMI
    ACPI_MADT_LOCAL_ADDR   = 5,  ///< Local APIC Address Override
    ACPI_MADT_IO_SAPIC     = 6,  ///< I/O SAPIC
    ACPI_MADT_LOCAL_SAPIC  = 7,  ///< Local SAPIC
    ACPI_MADT_PLATFORM_IS  = 8,  ///< Platform Interrupt Sources
    ACPI_MADT_LOCAL_X2APIC = 9,  ///< Processor Local x2APIC
    ACPI_MADT_X2APIC_NMI   = 10, ///< Local x2APIC NMI
    ACPI_MADT_GIC          = 11, ///< GIC
    ACPI_MADT_GICD         = 12, ///< GICD
};

//----------------------------------------------------------------------------
//  @struct     acpi_madt_hdr
/// @brief      MADT entry header.
//----------------------------------------------------------------------------
struct acpi_madt_hdr
{
    uint8_t type;           ///< acpi_madt_type
    uint8_t length;         ///< Length of IC structure including header
} PACKSTRUCT;

//----------------------------------------------------------------------------
//  @struct     acpi_madt_local_apic
/// @brief      MADT local APIC entry
//----------------------------------------------------------------------------
struct acpi_madt_local_apic
{
    struct acpi_madt_hdr hdr; // type = 0

    uint8_t  procid;          ///< Processor ID
    uint8_t  apicid;          ///< Local APIC ID
    uint32_t flags;           ///< Local APIC flags (bit 0 = usable)
} PACKSTRUCT;

//----------------------------------------------------------------------------
//  @struct     acpi_madt_local_apic
/// @brief      MADT I/O APIC entry
//----------------------------------------------------------------------------
struct acpi_madt_io_apic
{
    struct acpi_madt_hdr hdr; // type = 1

    uint8_t  apicid;          ///< I/O APIC ID
    uint8_t  reserved;
    uint32_t ptr_io_apic;     ///< I/O APIC address
    uint32_t interrupt_base;  ///< Interrupt # where interrupts start
} PACKSTRUCT;

//----------------------------------------------------------------------------
//  @struct     acpi_madt_local_apic
/// @brief      MADT Interrupt Source Override (ISO) entry
//----------------------------------------------------------------------------
struct acpi_madt_iso
{
    struct acpi_madt_hdr hdr; // type = 2

    uint8_t  bus;             ///< Must be 0, meaning ISA
    uint8_t  source;          ///< Bus-relative source (IRQ)
    uint32_t interrupt;       ///< Interrupt this soruce will signal
    uint16_t flags;           ///< MPS INTI flags
} PACKSTRUCT;

//----------------------------------------------------------------------------
//  @function   acpi_init
/// @brief      Find and parse all available ACPI tables.
//----------------------------------------------------------------------------
void
acpi_init();

//----------------------------------------------------------------------------
//  @function   acpi_version
/// @brief      Return the ACPI version number.
/// @returns    The ACPI version number (1 through 5).
//----------------------------------------------------------------------------
int
acpi_version();

//----------------------------------------------------------------------------
//  @function   acpi_fadt
/// @brief      Return a pointer to the ACPI fixed ACPI description table.
/// @returns    A pointer to the non-extended FADT structure.
//----------------------------------------------------------------------------
const struct acpi_fadt *
acpi_fadt();

//----------------------------------------------------------------------------
//  @function   acpi_madt
/// @brief      Return a pointer to the ACPI multiple APIC description table
///             (MADT).
/// @returns    A pointer to the MADT structure.
//----------------------------------------------------------------------------
const struct acpi_madt *
acpi_madt();

//----------------------------------------------------------------------------
//  @function   acpi_next_local_apic
/// @brief      Return a pointer to the next Local APIC structure entry in
///             the MADT table.
/// @param[in]  prev    Pointer to the local APIC returned by a previous call
///                     to this function. Pass NULL for the first call.
/// @returns    A pointer to a the next local APIC structure, or NULL if none
///             remain.
//----------------------------------------------------------------------------
const struct acpi_madt_local_apic *
acpi_next_local_apic(const struct acpi_madt_local_apic *prev);

//----------------------------------------------------------------------------
//  @function   acpi_next_io_apic
/// @brief      Return a pointer to the next I/O APIC structure entry in
///             the MADT table.
/// @param[in]  prev    Pointer to the I/O APIC returned by a previous call
///                     to this function. Pass NULL for the first call.
/// @returns    A pointer to a the next I/O APIC structure, or NULL if none
///             remain.
//----------------------------------------------------------------------------
const struct acpi_madt_io_apic *
acpi_next_io_apic(const struct acpi_madt_io_apic *prev);

//----------------------------------------------------------------------------
//  @function   acpi_next_iso
/// @brief      Return a pointer to the next Interrupt Source Override (ISO)
///             structure entry in the MADT table.
/// @param[in]  prev    Pointer to the ISO returned by a previous call
///                     to this function. Pass NULL for the first call.
/// @returns    A pointer to a the next ISO structure, or NULL if none remain.
//----------------------------------------------------------------------------
const struct acpi_madt_iso *
acpi_next_iso(const struct acpi_madt_iso *prev);
