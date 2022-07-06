#include "arch/x86/asm.h"

#include <stdint.h>
#include <stdio.h>

#define PCI_ADDR 0xCF8
#define PCI_DATA 0xCFC

union pci_header
{
	struct
	{
		uint16_t vendor;
		uint16_t device;
		uint16_t command;
		uint16_t status;
		uint8_t revision;
		uint8_t progif;
		uint8_t subclass;
		uint8_t class;
		uint8_t cacheline;
		uint8_t latency;
		uint8_t headertype;
		uint8_t bist;
	};
	struct
	{
		uint32_t v0;
		uint32_t v4;
		uint32_t v8;
		uint32_t vC;
	};
};

static uint32_t pci_read(uint32_t addr)
{
	outl(PCI_ADDR, addr);
	return inl(PCI_DATA);
}

static void checkdev(uint8_t bus, uint8_t slot, uint8_t func)
{
	uint32_t base = 0x80000000 | (bus << 16) | (slot << 11) | (func << 8);
	union pci_header header;
	header.v0 = pci_read(base + 0x0);
	if (header.vendor == 0xFFFF)
		return;
	header.v4 = pci_read(base + 0x4);
	header.v8 = pci_read(base + 0x8);
	header.vC = pci_read(base + 0xC);
	const char *name = "";
	if (header.vendor == 0x8086 && header.device == 0x100E)
		name = "82540EM Gigabit Ethernet Controller";
	else if (header.vendor == 0x8086 && header.device == 0x1237)
		name = "440FX - 82441FX PMC";
	else if (header.vendor == 0x8086 && header.device == 0x24CD)
		name = "82801DB/DBM USB2 EHCI Controller";
	else if (header.vendor == 0x8086 && header.device == 0x7000)
		name = "82371SB PIIX3 ISA";
	else if (header.vendor == 0x8086 && header.device == 0x7010)
		name = "82371SB PIIX3 IDE";
	else if (header.vendor == 0x8086 && header.device == 0x7020)
		name = "82371SB PIIX3 USB";
	else if (header.vendor == 0x8086 && header.device == 0x7113)
		name = "82371AB/EB/MB PIIX4 ACPI";
	else if (header.vendor == 0x1234 && header.device == 0x1111)
		name = "QEMU Virtual Video Controller";
	else if (header.vendor == 0x1B36 && header.device == 0x000D)
		name = "QEMU XHCI Host Controller";
	printf("%02x:%02x.%01x %08lx %08lx %08lx %08lx %s\n", bus, slot, func, header.v0, header.v4, header.v8, header.vC, name);
}

void pci_init(void)
{
	for (uint32_t bus = 0; bus < 256; ++bus)
	{
		for (uint32_t slot = 0; slot < 32; ++slot)
		{
			for (uint32_t func = 0; func < 8; ++func)
			{
				checkdev(bus, slot, func);
			}
		}
	}
}
