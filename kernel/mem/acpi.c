//============================================================================
/// @file       acpi.c
/// @brief      Advanced configuration and power interface (ACPI) tables.
//
//  Copyright 2016 Brett Vickers.
//  Use of this source code is governed by a BSD-style license
//  that can be found in the MonkOS LICENSE file.
//============================================================================

#include <kernel/debug/log.h>
#include <kernel/mem/acpi.h>
#include <kernel/mem/map.h>
#include <kernel/mem/paging.h>
#include <kernel/x86/cpu.h>

#define SIGNATURE_RSDP  0x2052545020445352ll    // "RSD PTR "
#define SIGNATURE_FADT  0x50434146              // "FACP"
#define SIGNATURE_BOOT  0x544f4f42              // "BOOT"
#define SIGNATURE_MADT  0x43495041              // "APIC"
#define SIGNATURE_MCFG  0x4746434d              // "MCFG"
#define SIGNATURE_SRAT  0x54415253              // "SRAT"
#define SIGNATURE_HPET  0x54455048              // "HPET"
#define SIGNATURE_WAET  0x54454157              // "WAET"
#define SIGNATURE_SSDT  0x54445353              // "SSDT"

struct acpi_rsdp
{
    char     signature[8]; ///< Contains "RSD PTR "
    uint8_t  checksum;     ///< Covers up to (and including) ptr_rsdt
    char     oemid[6];     ///< Supplied by the OEM
    uint8_t  revision;     ///< 0=1.0, 1=2.0, 2=3.0
    uint32_t ptr_rsdt;     ///< 32-bit pointer to RSDT table

    // The following fields do not exist in ACPI1.0
    uint32_t length;       ///< RSDT table length, including header
    uint64_t ptr_xsdt;     ///< 64-bit pointer to XSDT table
    uint8_t  checksum_ex;  ///< Covers entire rsdp structure
    uint8_t  reserved[3];
};

struct acpi_rsdt
{
    struct acpi_hdr hdr;
    uint32_t        ptr_table[1]; ///< Pointers to other ACPI tables
} PACKSTRUCT;

struct acpi_xsdt
{
    struct acpi_hdr hdr;
    uint64_t        ptr_table[1]; ///< Pointers to other ACPI tables
} PACKSTRUCT;

struct acpi
{
    int                     version; // ACPI version (1, 2 or 3)
    const struct acpi_rsdp *rsdp;
    const struct acpi_rsdt *rsdt;
    const struct acpi_xsdt *xsdt;
    const struct acpi_fadt *fadt;
    const struct acpi_madt *madt;
};

static struct acpi acpi;

static void
read_fadt(const struct acpi_hdr *hdr)
{
    const struct acpi_fadt *fadt = (const struct acpi_fadt *)hdr;
    acpi.fadt = fadt;
}

static void
read_madt(const struct acpi_hdr *hdr)
{
    const struct acpi_madt *madt = (const struct acpi_madt *)hdr;
    acpi.madt = madt;
}

static void
read_table(const struct acpi_hdr *hdr)
{
    uint32_t sig = *(const uint32_t *)hdr->signature;
    switch (sig)
    {
        case SIGNATURE_FADT:
            read_fadt(hdr);

        case SIGNATURE_MADT:
            read_madt(hdr);

        default:
            break;
    }
}

static void
read_xsdt()
{
    const struct acpi_xsdt *xsdt = acpi.xsdt;
    const struct acpi_hdr  *xhdr = &xsdt->hdr;

    logf(LOG_INFO,
         "[acpi] oem='%.6s' tbl='%.8s' rev=%#x creator='%.4s'",
         xhdr->oemid, xhdr->oemtableid, xhdr->oemrevision, xhdr->creatorid);

    int tables = (int)(xhdr->length - sizeof(*xhdr)) / sizeof(uint64_t);
    for (int i = 0; i < tables; i++) {
        const struct acpi_hdr *hdr =
            (const struct acpi_hdr *)xsdt->ptr_table[i];
        map_range((uint64_t)hdr, sizeof(struct acpi_hdr));
        map_range((uint64_t)hdr, hdr->length);
        logf(LOG_INFO, "[acpi] Found %.4s table at %#lx.",
             hdr->signature, (uint64_t)hdr);
        read_table(hdr);
    }
}

static void
read_rsdt()
{
    const struct acpi_rsdt *rsdt = acpi.rsdt;
    const struct acpi_hdr  *rhdr = &rsdt->hdr;

    logf(LOG_INFO,
         "[acpi] oem='%.6s' tbl='%.8s' rev=%#x creator='%.4s'",
         rhdr->oemid, rhdr->oemtableid, rhdr->oemrevision, rhdr->creatorid);

    int tables = (int)(rhdr->length - sizeof(*rhdr)) / sizeof(uint32_t);
    for (int i = 0; i < tables; i++) {
        const struct acpi_hdr *hdr =
            (const struct acpi_hdr *)(uintptr_t)rsdt->ptr_table[i];
        map_range((uint64_t)hdr, sizeof(struct acpi_hdr));
        map_range((uint64_t)hdr, hdr->length);
        logf(LOG_INFO, "[acpi] Found %.4s table at %#lx.",
             hdr->signature, (uint64_t)hdr);
        read_table(hdr);
    }
}

static const struct acpi_rsdp *
find_rsdp(uint64_t addr, uint64_t size)
{
    const uint64_t *ptr  = (const uint64_t *)addr;
    const uint64_t *term = (const uint64_t *)(addr + size);
    for (; ptr < term; ptr += 2) {
        if (*ptr == SIGNATURE_RSDP)
            return (const struct acpi_rsdp *)ptr;
    }
    return NULL;
}

void
acpi_init()
{
    // Scan the extended BIOS and system ROM memory regions for the ACPI
    // RSDP table.
    acpi.rsdp = find_rsdp(MEM_EXTENDED_BIOS, MEM_EXTENDED_BIOS_SIZE);
    if (acpi.rsdp == NULL)
        acpi.rsdp = find_rsdp(MEM_SYSTEM_ROM, MEM_SYSTEM_ROM_SIZE);

    // Fatal out if the ACPI tables could not be found.
    if (acpi.rsdp == NULL) {
        logf(LOG_CRIT, "[acpi] No ACPI tables found.");
        fatal();
    }

    acpi.version = acpi.rsdp->revision + 1;
    logf(LOG_INFO, "[acpi] ACPI %d.0 RSDP table found at %#lx.",
         acpi.version, (uintptr_t)acpi.rsdp);

    // Prefer the ACPI2.0 XSDT table for finding all other tables.
    if (acpi.version > 1) {
        acpi.xsdt = (const struct acpi_xsdt *)acpi.rsdp->ptr_xsdt;
        if (acpi.xsdt == NULL) {
            logf(LOG_INFO, "[acpi] No XSDT table found.");
        }
        else {
            logf(LOG_INFO, "[acpi] Found XSDT table at %#lx.",
                 (uintptr_t)acpi.xsdt);
            map_range((uint64_t)acpi.xsdt, sizeof(struct acpi_xsdt));
            read_xsdt();
            return;
        }
    }

    // Fall back to the ACPI1.0 RSDT table if XSDT isn't available.
    acpi.rsdt = (const struct acpi_rsdt *)(uintptr_t)acpi.rsdp->ptr_rsdt;
    if (acpi.rsdt == NULL) {
        logf(LOG_CRIT, "[acpi] No RSDT table found.");
        fatal();
    }
    else {
        logf(LOG_INFO, "[acpi] Found RSDT table at %#lx.",
             (uintptr_t)acpi.rsdt);
        map_range((uint64_t)acpi.rsdt, sizeof(struct acpi_rsdt));
        read_rsdt();
    }
}

int
acpi_version()
{
    return acpi.version;
}

const struct acpi_fadt *
acpi_fadt()
{
    return acpi.fadt;
}

const struct acpi_madt *
acpi_madt()
{
    return acpi.madt;
}

static const void *
madt_find(enum acpi_madt_type type, const void *prev)
{
    const struct acpi_madt *madt = acpi.madt;
    if (madt == NULL)
        return NULL;

    const void *term = (const uint8_t *)madt + madt->hdr.length;

    const void *ptr;
    if (prev == NULL) {
        ptr = madt + 1;
    }
    else {
        ptr = (const uint8_t *)prev +
              ((const struct acpi_madt_hdr *)prev)->length;
    }

    while (ptr < term) {
        const struct acpi_madt_hdr *hdr = (const struct acpi_madt_hdr *)ptr;
        if (hdr->type == type)
            return hdr;
        ptr = (const uint8_t *)hdr + hdr->length;
    }

    return NULL;
}

const struct acpi_madt_local_apic *
acpi_next_local_apic(const struct acpi_madt_local_apic *prev)
{
    return (const struct acpi_madt_local_apic *)madt_find(
        ACPI_MADT_LOCAL_APIC, prev);
}

const struct acpi_madt_io_apic *
acpi_next_io_apic(const struct acpi_madt_io_apic *prev)
{
    return (const struct acpi_madt_io_apic *)madt_find(
        ACPI_MADT_IO_APIC, prev);
}

const struct acpi_madt_iso *
acpi_next_iso(const struct acpi_madt_iso *prev)
{
    return (const struct acpi_madt_iso *)madt_find(
        ACPI_MADT_ISO, prev);
}
