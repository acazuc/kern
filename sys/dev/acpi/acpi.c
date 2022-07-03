#include "acpi.h"
#include "arch/x86/x86.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>
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
} __attribute__ ((packed));

struct rsdt
{
	struct acpi_hdr hdr;
	uint32_t data[];
};

struct madt
{
	struct acpi_hdr hdr;
	uint32_t apic_addr;
	uint32_t flags;
} __attribute__ ((packed));

struct madt_entry
{
	uint8_t type;
	uint8_t length;
} __attribute__ ((packed));

enum madt_type
{
	MADT_LOCAL_APIC               = 0x0,
	MADT_IO_APIC                  = 0x1,
	MADT_INT_SRC_OVERRIDE         = 0x2,
	MADT_NMI_SRC                  = 0x3,
	MADT_LOCAL_APIC_NMI           = 0x4,
	MADT_LOCAL_ACIC_ADDR_OVERRIDE = 0x5,
	MADT_IO_SAPIC                 = 0x6,
	MADT_LOCAL_SAPIC              = 0x7,
	MADT_PLATFORM_INT_SOURCE      = 0x8,
	MADT_LOCAL_X2APIC             = 0x9,
	MADT_LOCAL_X2APIC_NMI         = 0xA,
	MADT_GICC                     = 0xB,
	MADT_GICD                     = 0xC,
	MADT_GIC_MSI_FRAME            = 0xD,
	MADT_GICCR                    = 0xE,
	MADT_ITS                      = 0xF,
	MADT_RESERVED                 = 0x10, /* 0x10 - 0x7F */
	MADT_OEM                      = 0x80, /* 0x80 - 0xFF */
};

struct madt_local_apic
{
	struct madt_entry entry;
	uint8_t acpi_cpuid;
	uint8_t apic_id;
	uint32_t flags;
} __attribute__ ((packed));

enum madt_local_apic_flags
{
	MADT_LOCAL_APIC_F_ENABLED = 0x1,
	MADT_LOCAL_APIC_F_CAPABLE = 0x2,
};

struct madt_io_apic
{
	struct madt_entry entry;
	uint8_t apic_id;
	uint8_t reserved;
	uint32_t apic_addr;
	uint32_t gsib;
} __attribute__ ((packed));

struct madt_int_src_override
{
	struct madt_entry entry;
	uint8_t bus;
	uint8_t source;
	uint32_t gsi;
	uint16_t flags;
} __attribute__ ((packed));

enum madt_int_src_override_flags
{
	MADT_INT_SRC_OVERRIDE_F_POLARITY_CONFORM = 0x0,
	MADT_INT_SRC_OVERRIDE_F_POLARITY_HIGH    = 0x1,
	MADT_INT_SRC_OVERRIDE_F_POLARITY_LOW     = 0x3,
	MADT_INT_SRC_OVERRIDE_F_POLARITY_MASK    = 0x3,
	MADT_INT_SRC_OVERRIDE_F_TRIGGER_CONFORM  = 0x0,
	MADT_INT_SRC_OVERRIDE_F_TRIGGER_EDGE     = 0x4,
	MADT_INT_SRC_OVERRIDE_F_TRIGGER_LEVEL    = 0xC,
	MADT_INT_SRC_OVERRIDE_F_TRIGGER_MASK     = 0xC
};

enum acpi_address_space_id
{
	ACPI_ADDR_SYSTEM_MEM = 0x0,
	ACPI_ADDR_SYSTEM_IO  = 0x1,
	ACPI_ADDR_PCI_CONF   = 0x2,
	ACPI_ADDR_EMBED_CTRL = 0x3,
	ACPI_ADDR_SMBUS      = 0x4,
	ACPI_ADDR_SYSTEMCMOS = 0x5,
	ACPI_ADDR_PRICBAR    = 0x6,
	ACPI_ADDR_IPMI       = 0x7,
	ACPI_ADDR_GPIO       = 0x8,
	ACPI_ADDR_GEN_SERIAL = 0x9,
	ACPI_ADDR_PCC        = 0xA,
	ACPI_ADDR_FIXED_HARD = 0x7F,
	ACPI_ADDR_RESERVED   = 0x80, /* 0x80 - 0xBF */
	ACPI_ADDR_OEM        = 0xC0, /* 0xC0 - 0xFF */
};

struct acpi_gas
{
	uint8_t address_space_id;
	uint8_t bit_width;
	uint8_t bit_offset;
	uint8_t access_size;
	uint64_t address;
};

struct fadt
{
	struct acpi_hdr hdr;
	uint32_t firmware_ctrl;
	uint32_t dsdt;
	uint8_t reserved;
	uint8_t preferred_pm_profile;
	uint16_t sci_int;
	uint32_t smi_cmd;
	uint8_t acpi_enable;
	uint8_t acpi_disable;
	uint8_t s4bios_req;
	uint8_t pstate_cnt;
	uint32_t pm1a_evt_blk;
	uint32_t pm1b_evt_blk;
	uint32_t pm1a_cnt_blk;
	uint32_t pm1b_cnt_blk;
	uint32_t pm2_cnt_blk;
	uint32_t pm_tmr_blk;
	uint32_t gpe0_blk;
	uint32_t gpe1_blk;
	uint8_t pm1_evt_len;
	uint8_t pm1_cnt_len;
	uint8_t pm2_cnt_len;
	uint8_t pm_tmr_len;
	uint8_t gpe0_blk_len;
	uint8_t gpe1_blk_len;
	uint8_t gpe1_base;
	uint8_t cst_cnt;
	uint16_t p_lvl2_lat;
	uint16_t p_lvl3_lat;
	uint16_t flush_size;
	uint16_t flush_stride;
	uint8_t duty_offset;
	uint8_t duty_width;
	uint8_t day_alrm;
	uint8_t month_alrm;
	uint8_t century;
	uint16_t iapc_boot_arch;
	uint8_t  reserved2;
	uint32_t flags;
	struct acpi_gas reset_reg;
	uint8_t reset_value;
	uint16_t arm_boot_arch;
	uint8_t minor_version;
	uint64_t x_firmware_control;
	uint64_t x_dsdt;
	struct acpi_gas x_pm1a_evt_blk;
	struct acpi_gas x_pm1b_evt_blk;
	struct acpi_gas x_pm1a_cnt_blk;
	struct acpi_gas x_pm1b_cnt_blk;
	struct acpi_gas x_pm2_cnt_blk;
	struct acpi_gas x_pm_tmr_blk;
	struct acpi_gas x_gpe0_blk;
	struct acpi_gas x_gpe1_blk;
	struct acpi_gas sleep_control_reg;
	struct acpi_gas sleep_status_reg;
	uint64_t hypervisor_vendor_identity;
};

/*
 * one local apic per vcpu
 * one io apic per package ?
 *
 * XXX: start mp
 * XXX: reloc 8259 int to IO/APIC
 * XXX: symmetric IO mode
 */

#define ACPI_NAME(a, b, c, d) (((a) << 0) | ((b) << 8) | ((c) << 16) | ((d) << 24))

static uint8_t acpi_table_checksum(const struct acpi_hdr *hdr)
{
	uint8_t checksum = 0;
	for (size_t i = 0; i < hdr->length; ++i)
		checksum += ((const uint8_t*)hdr)[i];
	return checksum;
}

static const void *map_table(uint32_t addr)
{
	uint32_t addr_base = addr & ~0xFFF;
	uint32_t addr_diff = addr - addr_base;
	uint32_t addr_size = 4096; /* XXX: handle multi-page; handle already-mapped pmem; unmap */
	const void *map_base = vmap(addr_base, addr_size);
	if (!map_base)
		return NULL;
	return ((const uint8_t*)map_base + addr_diff);
}

static const void *find_table(const struct rsdt *rsdt, const char *name)
{
	for (uint32_t i = 0; i < rsdt->hdr.length / 4; ++i)
	{
		if (!rsdt->data[i])
			continue;
		const struct acpi_hdr *header = map_table(rsdt->data[i]);
		if (!memcmp(header->signature, name, 4))
			return header;
	}
	return NULL;
}

static void handle_madt(const struct madt *madt)
{
	uint8_t checksum = acpi_table_checksum(&madt->hdr);
	assert(!checksum, "invalid madt checksum: %02x\n", checksum);
	struct madt_entry *entry = (struct madt_entry*)((uint8_t*)madt + sizeof(*madt));
	do
	{
		switch (entry->type)
		{
			case MADT_LOCAL_APIC:
			{
				struct madt_local_apic *local_apic = (struct madt_local_apic*)entry;
				printf("local apic: %u %u %lx\n", local_apic->acpi_cpuid, local_apic->apic_id, local_apic->flags);
				break;
			}
			case MADT_IO_APIC:
			{
				struct madt_io_apic *io_apic = (struct madt_io_apic*)entry;
				printf("io apic: %u, %lx, %lx\n", io_apic->apic_id, io_apic->apic_addr, io_apic->gsib);
				break;
			}
			case MADT_INT_SRC_OVERRIDE:
			{
				struct madt_int_src_override *src_override = (struct madt_int_src_override*)entry;
				printf("int src override: %d, %lx, %lx\n", src_override->source, src_override->gsi, src_override->flags);
				break;
			}
			case MADT_LOCAL_APIC_NMI:
			{
				struct madt_local_apic_nmi *apic_nmi = (struct madt_local_apic_nmi*)entry;
				printf("local apic nmi: \n");
				break;
			}
			default:
				panic("unhandled madt entry type: %x\n", entry->type);
		}
		entry = (struct madt_entry*)((uint8_t*)entry + entry->length);
	} while ((uint8_t*)entry < (uint8_t*)madt + madt->hdr.length);
}

static void handle_rsdt(const struct rsdt *rsdt)
{
	uint8_t checksum = acpi_table_checksum(&rsdt->hdr);
	assert(!checksum, "invalid rsdt checksum: %02x\n", checksum);
	const struct fadt *fadt = find_table(rsdt, "FACP");
	printf("fadt: %p\n", fadt);
	const struct madt *madt = find_table(rsdt, "APIC");
	handle_madt(madt);
}

void acpi_handle_rsdp(const struct rsdp *rsdp)
{
	uint8_t checksum = 0;
	for (size_t i = 0; i < sizeof(*rsdp); ++i)
		checksum += ((const uint8_t*)rsdp)[i];
	assert(!checksum, "invalid rsdp checksum: %02x\n", checksum);
	uint32_t rsdt_addr;
	if (rsdp->revision >= 2)
	{
		const struct rsdp2 *rsdp2 = (const struct rsdp2*)rsdp;
		uint8_t checksum = 0;
		for (size_t i = 0; i < sizeof(*rsdp) + sizeof(*rsdp2); ++i)
			checksum += ((const uint8_t*)rsdp)[i];
		assert(!checksum, "invalid rsdp2 checksum: %02x\n", checksum);
		rsdt_addr = rsdp2->xsdt;
	}
	else
	{
		rsdt_addr = rsdp->rsdt;
	}
	assert(rsdt_addr, "rsdt not found\n");
	const struct rsdt *rsdt = map_table(rsdt_addr);
	assert(rsdt, "can't vmap rsdt\n");
	handle_rsdt(rsdt);
}

const void *acpi_find_rsdp(void)
{
	for (const uint32_t *addr = (const uint32_t*)0xE0000; addr < (const uint32_t*)0x100000; addr += 4)
	{
		if (addr[0] != 0x20445352 || addr[1] != 0x20525450)
			continue;
		return addr;
	}
	return NULL;
}
