//============================================================================
/// @file       acpi.c
/// @brief      Advanced configuration and power interface (ACPI) tables.
//
//  Copyright 2016 Brett Vickers.
//  Use of this source code is governed by a BSD-style license
//  that can be found in the MonkOS LICENSE file.
//============================================================================

#include <core.h>
#include <libc/string.h>
#include <kernel/debug/log.h>
#include <kernel/mem/acpi.h>
#include <kernel/mem/map.h>
#include <kernel/mem/paging.h>
#include <kernel/mem/table.h>
#include <kernel/x86/cpu.h>

#define SIGNATURE_RSDP      0x2052545020445352ll // "RSD PTR "
#define SIGNATURE_MADT      0x43495041           // "APIC"
#define SIGNATURE_BOOT      0x544f4f42           // "BOOT"
#define SIGNATURE_FADT      0x50434146           // "FACP"
#define SIGNATURE_HPET      0x54455048           // "HPET"
#define SIGNATURE_MCFG      0x4746434d           // "MCFG"
#define SIGNATURE_SRAT      0x54415253           // "SRAT"
#define SIGNATURE_SSDT      0x54445353           // "SSDT"
#define SIGNATURE_WAET      0x54454157           // "WAET"

// Page alignment macros
#define PAGE_ALIGN_DOWN(a)  ((a) & ~(PAGE_SIZE - 1))
#define PAGE_ALIGN_UP(a)    (((a) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

/// A structure used to track the state of the temporary page table generated
/// by the boot loader. The ACPI code updates it to access the memory stored
/// in ACPI tables.
typedef struct btable
{
    page_t *root;                ///< The top-level (PML4) page table
    page_t *next_page;           ///< The next page to use when allocating
    page_t *term_page;           ///< Just beyond the last available page
} btable_t;

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
} PACKSTRUCT;

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
            break;

        case SIGNATURE_MADT:
            read_madt(hdr);
            break;

        default:
            break;
    }
}

static bool
is_mapped(btable_t *btable, uint64_t addr)
{
    uint64_t pml4te = PML4E(addr);
    uint64_t pdpte  = PDPTE(addr);
    uint64_t pde    = PDE(addr);
    uint64_t pte    = PTE(addr);

    page_t *pml4t = btable->root;
    if (pml4t->entry[pml4te] == 0)
        return false;

    page_t *pdpt = PGPTR(pml4t->entry[pml4te]);
    if (pdpt->entry[pdpte] == 0)
        return false;
    if (pdpt->entry[pdpte] & PF_PS)
        return true;

    page_t *pdt = PGPTR(pdpt->entry[pdpte]);
    if (pdt->entry[pde] == 0)
        return false;
    if (pdt->entry[pde] & PF_PS)
        return true;

    page_t *pt = PGPTR(pdt->entry[pde]);
    return pt->entry[pte] != 0;
}

static uint64_t
alloc_page(btable_t *btable)
{
    if (btable->next_page == btable->term_page)
        fatal();

    page_t *page = btable->next_page++;
    memzero(page, sizeof(page_t));
    return (uint64_t)page | PF_PRESENT | PF_RW;
}

static void
create_page(btable_t *btable, uint64_t addr, uint64_t flags)
{
    uint64_t pml4te = PML4E(addr);
    uint64_t pdpte  = PDPTE(addr);
    uint64_t pde    = PDE(addr);
    uint64_t pte    = PTE(addr);

    page_t *pml4t = btable->root;
    if (pml4t->entry[pml4te] == 0)
        pml4t->entry[pml4te] = alloc_page(btable);

    page_t *pdpt = PGPTR(pml4t->entry[pml4te]);
    if (pdpt->entry[pdpte] == 0)
        pdpt->entry[pdpte] = alloc_page(btable);

    page_t *pdt = PGPTR(pdpt->entry[pdpte]);
    if (pdt->entry[pde] == 0)
        pdt->entry[pde] = alloc_page(btable);

    page_t *pt = PGPTR(pdt->entry[pde]);
    pt->entry[pte] = addr | flags;
}

static void
map_range(btable_t *btable, uint64_t addr, uint64_t size, uint64_t flags)
{
    // Calculate the page-aligned extents of the block of memory.
    uint64_t begin = PAGE_ALIGN_DOWN(addr);
    uint64_t term  = PAGE_ALIGN_UP(addr + size);

    // If necessary, create new page in the boot page table to cover the
    // pages.
    for (uint64_t addr = begin; addr < term; addr += PAGE_SIZE) {
        if (!is_mapped(btable, addr))
            create_page(btable, addr, flags);
    }
}

static void
map_table(btable_t *btable, const struct acpi_hdr *hdr)
{
    uint64_t addr  = (uint64_t)hdr;
    uint64_t flags = PF_PRESENT | PF_RW;

    // First map the header itself, since we can't read its length until
    // it's mapped.
    map_range(btable, addr, sizeof(struct acpi_hdr), flags);

    // Now that we can read the header's length, map the entire ACPI table.
    map_range(btable, addr, hdr->length, flags);

    // Calculate the page-aligned extents of the ACPI table, and add them to
    // the BIOS-generated memory table.
    memtable_add(
        PAGE_ALIGN_DOWN(addr),
        PAGE_ALIGN_UP(addr + hdr->length) - PAGE_ALIGN_DOWN(addr),
        MEMTYPE_UNCACHED);
}

static void
read_xsdt(btable_t *btable)
{
    const struct acpi_xsdt *xsdt = acpi.xsdt;
    const struct acpi_hdr  *xhdr = &xsdt->hdr;

    logf(LOG_INFO,
         "[acpi] oem='%.6s' tbl='%.8s' rev=%#x creator='%.4s'",
         xhdr->oemid, xhdr->oemtableid, xhdr->oemrevision, xhdr->creatorid);

    // Read each of the tables referenced by the XSDT table.
    int tables = (int)(xhdr->length - sizeof(*xhdr)) / sizeof(uint64_t);
    for (int i = 0; i < tables; i++) {
        const struct acpi_hdr *hdr =
            (const struct acpi_hdr *)xsdt->ptr_table[i];
        map_table(btable, hdr);
        logf(LOG_INFO, "[acpi] Found %.4s table at %#lx.",
             hdr->signature, (uint64_t)hdr);
        read_table(hdr);
    }
}

static void
read_rsdt(btable_t *btable)
{
    const struct acpi_rsdt *rsdt = acpi.rsdt;
    const struct acpi_hdr  *rhdr = &rsdt->hdr;

    logf(LOG_INFO,
         "[acpi] oem='%.6s' tbl='%.8s' rev=%#x creator='%.4s'",
         rhdr->oemid, rhdr->oemtableid, rhdr->oemrevision, rhdr->creatorid);

    // Read each of the tables referenced by the RSDT table.
    int tables = (int)(rhdr->length - sizeof(*rhdr)) / sizeof(uint32_t);
    for (int i = 0; i < tables; i++) {
        const struct acpi_hdr *hdr =
            (const struct acpi_hdr *)(uintptr_t)rsdt->ptr_table[i];
        map_table(btable, hdr);
        logf(LOG_INFO, "[acpi] Found %.4s table at %#lx.",
             hdr->signature, (uint64_t)hdr);
        read_table(hdr);
    }
}

static const struct acpi_rsdp *
find_rsdp(uint64_t addr, uint64_t size)
{
    // Scan memory for the 8-byte RSDP signature. It's guaranteed to be
    // aligned on a 16-byte boundary.
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
    // Initialize the state of the temporary page table generated by the boot
    // loader. We'll be updating it as we scan ACPI tables.
    btable_t btable =
    {
        .root      = (page_t *)MEM_BOOT_PAGETABLE,
        .next_page = (page_t *)MEM_BOOT_PAGETABLE_LOADED,
        .term_page = (page_t *)MEM_BOOT_PAGETABLE_END,
    };

    // Scan the extended BIOS and system ROM memory regions for the ACPI RSDP
    // table.
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
            map_table(&btable, &acpi.xsdt->hdr);
            read_xsdt(&btable);
        }
    }

    // Fall back to the ACPI1.0 RSDT table if XSDT isn't available.
    if (acpi.xsdt == NULL) {
        acpi.rsdt = (const struct acpi_rsdt *)(uintptr_t)acpi.rsdp->ptr_rsdt;
        if (acpi.rsdt == NULL) {
            logf(LOG_CRIT, "[acpi] No RSDT table found.");
            fatal();
        }
        else {
            logf(LOG_INFO, "[acpi] Found RSDT table at %#lx.",
                 (uintptr_t)acpi.rsdt);
            map_table(&btable, &acpi.rsdt->hdr);
            read_rsdt(&btable);
        }
    }

    // Reserve local APIC memory-mapped I/O addresses.
    if (acpi.madt != NULL) {
        memtable_add(PAGE_ALIGN_DOWN(acpi.madt->ptr_local_apic), PAGE_SIZE,
                     MEMTYPE_UNCACHED);
    }

    // Reserve I/O APIC memory-mapped I/O addresses.
    const struct acpi_madt_io_apic *io = NULL;
    while ((io = acpi_next_io_apic(io)) != NULL) {
        memtable_add(PAGE_ALIGN_DOWN(io->ptr_io_apic), PAGE_SIZE,
                     MEMTYPE_UNCACHED);
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
