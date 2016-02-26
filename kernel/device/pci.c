//============================================================================
/// @file       pci.c
/// @brief      PCI controller.
//
// Copyright 2016 Brett Vickers.
// Use of this source code is governed by a BSD-style license that can
// be found in the MonkOS LICENSE file.
//============================================================================

#include <core.h>
#include <kernel/device/pci.h>
#include <kernel/device/tty.h>
#include <kernel/x86/cpu.h>

#define DEBUG_PCI        1

#define PCI_CONFIG_ADDR  0x0cf8
#define PCI_CONFIG_DATA  0x0cfc

static inline uint32_t
read(uint32_t bus, uint32_t device, uint32_t func, uint32_t offset)
{
    uint32_t addr = (1u << 31) |
                    (bus << 16) |
                    (device << 11) |
                    (func << 8) |
                    offset;

    io_outd(PCI_CONFIG_ADDR, addr);
    return io_ind(PCI_CONFIG_DATA);
}

static inline uint32_t
read_hdrtype(uint32_t bus, uint32_t device, uint32_t func)
{
    uint32_t value = read(bus, device, func, 0x0c);
    return (value >> 16) & 0xff;
}

static inline uint32_t
read_deviceid(uint32_t bus, uint32_t device, uint32_t func)
{
    uint32_t value = read(bus, device, func, 0x00);
    return (value >> 16) & 0xffff;
}

static inline uint32_t
read_vendor(uint32_t bus, uint32_t device, uint32_t func)
{
    uint32_t value = read(bus, device, func, 0x00);
    return value & 0xffff;
}

static inline uint32_t
read_class(uint32_t bus, uint32_t device, uint32_t func)
{
    uint32_t value = read(bus, device, func, 0x08);
    return value >> 24;
}

static inline uint32_t
read_subclass(uint32_t bus, uint32_t device, uint32_t func)
{
    uint32_t value = read(bus, device, func, 0x08);
    return (value >> 16) & 0xff;
}

static inline uint32_t
read_secondary_bus(uint32_t bus, uint32_t device, uint32_t func)
{
    uint32_t value = read(bus, device, func, 0x18);
    return (value >> 8) & 0xff;
}

static void probe_bus(uint32_t bus);

static bool
probe_function(uint32_t bus, uint32_t device, uint32_t func)
{
    // Validate the function
    uint32_t vendor = read_vendor(bus, device, func);
    if (vendor == 0xffff)
        return false;

    // Read the device's class/subclass
    uint32_t class    = read_class(bus, device, func);
    uint32_t subclass = read_subclass(bus, device, func);

    // Is this a PCI-to-PCI bridge device? If so, recursively scan the
    // bridge's secondary bus.
    if (class == 6 && subclass == 4) {
        uint32_t bus2 = read_secondary_bus(bus, device, func);
        probe_bus(bus2);
    }

#if DEBUG_PCI
    else {
        uint32_t devid = read_deviceid(bus, device, func);
        tty_printf(
            0,
            "[pci] %u/%u/%u vendor=0x%04x devid=0x%04x "
            "class=%02x subclass=%02x\n",
            bus, device, func, vendor, devid, class, subclass);
    }
#endif

    return true;
}

static void
probe_device(uint32_t bus, uint32_t device)
{
    // Probe device function 0.
    if (!probe_function(bus, device, 0))
        return;

    // Probe functions 1 through 8 if the device is multi-function.
    uint32_t hdrtype = read_hdrtype(bus, device, 0);
    if (hdrtype & 0x80) {
        for (uint32_t func = 1; func < 8; ++func)
            probe_function(bus, device, func);
    }
}

static void
probe_bus(uint32_t bus)
{
    // Probe all possible devices on the bus.
    for (uint32_t device = 0; device < 32; ++device)
        probe_device(bus, device);
}

void
pci_init()
{
    // Always probe bus 0.
    probe_bus(0);

    // If bus 0 device 0 is multi-function, probe remaining 7 buses.
    uint32_t hdrtype = read_hdrtype(0, 0, 0);
    if (hdrtype & 0x80) {
        for (uint32_t bus = 1; bus < 8; ++bus) {
            uint32_t vendor = read_vendor(0, 0, bus); // func = bus #
            if (vendor != 0xffff)
                probe_bus(bus);
        }
    }
}
