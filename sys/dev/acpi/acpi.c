#include "acpi.h"
#include "arch/x86/x86.h"

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

struct rsdp
{
	char signature[8];
	uint8_t checksum;
	char oem_id[6];
	uint8_t revision;
	uint32_t rsdt;
} __attribute__ ((packed));

struct rsdp2
{
	struct rsdp rsdp;
	uint32_t length;
	uint64_t xsdt;
	uint8_t ext_checksum;
	uint8_t reserved[3];
} __attribute__ ((packed));

struct acpi_hdr
{
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oem_id[6];
	char oem_tableid[8];
	uint32_t oem_revision;
	uint32_t creator_id;
	uint32_t creator_revision;
};

struct rsdt
{
	struct acpi_hdr hdr;
	uint8_t data[];
};

struct madt
{
	struct acpi_hdr hdr;
	uint32_t apic_addr;
	uint32_t flags;
};

static void handle_rsdt(struct rsdt *rsdt)
{
	uint8_t checksum = 0;
	for (size_t i = 0; i < rsdt->hdr.length; ++i)
		checksum += ((uint8_t*)rsdt)[i];
	if (checksum)
		panic("invalid rsdt checksum: %02x\n", checksum);
	printf("valid rsdt checksum\n");
}

void acpi_handle_rsdp(struct rsdp *rsdp)
{
	uint8_t checksum = 0;
	for (size_t i = 0; i < sizeof(*rsdp); ++i)
		checksum += ((uint8_t*)rsdp)[i];
	if (checksum)
		panic("invalid rsdp checksum: %02x\n", checksum);
	printf("valid rsdp checksum\n");
	uint32_t rsdt_addr;
	if (rsdp->revision == 2)
	{
		struct rsdp2 *rsdp2 = (struct rsdp2*)rsdp;
		uint8_t checksum = 0;
		for (size_t i = sizeof(*rsdp); i < sizeof(*rsdp) + sizeof(*rsdp2); ++i)
			checksum += ((uint8_t*)rsdp2)[i];
		if (checksum)
			panic("invalid rsdp2 checksum: %02x\n", checksum);
		printf("valid rsdp2 checksum\n");
		rsdt_addr = rsdp2->xsdt;
	}
	else
	{
		rsdt_addr = rsdp->rsdt;
	}
	if (!rsdt_addr)
		panic("rsdt not found\n");
	printf("rsdt: 0x%08lx\n", rsdt_addr);
	uint32_t rsdt_base = rsdt_addr & ~0xFFF;
	uint32_t rsdt_diff = rsdt_addr - rsdt_base;
	uint32_t rsdt_size = 4096; /* XXX: handle multiple-page rsdt */
	void *map_base = vmap(rsdt_base, rsdt_size);
	struct rsdt *rsdt = (struct rsdt*)((uint8_t*)map_base + rsdt_diff);
	if (!rsdt)
		panic("can't vmap rsdt\n");
	handle_rsdt(rsdt);
	vunmap(map_base, rsdt_size);
}

void *acpi_find_rsdp(void)
{
	for (uint32_t *addr = (uint32_t*)0xE0000; addr < (uint32_t*)0x100000; addr += 4)
	{
		if (addr[0] != 0x20445352 || addr[1] != 0x20525450)
			continue;
		return addr;
	}
	return NULL;
}
