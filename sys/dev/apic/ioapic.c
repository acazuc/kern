#include "apic.h"
#include "arch/arch.h"

#include <stddef.h>
#include <stdio.h>

#define IOAPIC_IOREGSEL 0x0
#define IOAPIC_IOWIN    0x4

#define IOAPIC_REG_ID     0x0
#define IOAPIC_REG_VER    0x1
#define IOAPIC_REG_ARB    0x2
#define IOAPIC_REG_REDTBL 0x10

#define IOAPIC_R_DST_MASK    (0xFFULL << 56)
#define IOAPIC_R_INT_MASK    (0x1     << 16)
#define IOAPIC_R_TRIG_MASK   (0x1     << 15)
#define IOAPIC_R_IRR_MASK    (0x1     << 14)
#define IOAPIC_R_INTPOL_MASK (0x1     << 13)
#define IOAPIC_R_DELIVS_MASK (0x1     << 12)
#define IOAPIC_R_DSTMOD_MASK (0x1     << 11)
#define IOAPIC_R_DELMOD_MASK (0x7     << 8)
#define IOAPIC_R_INTVEC_MASK (0xFF    << 0)

struct ioapic
{
	uint32_t volatile *data;
};

struct ioapic ioapic0;

void ioapic_enable_int(uint8_t id, uint8_t intid);

static uint32_t ioapic_rd(struct ioapic *ioapic, uint32_t reg)
{
	ioapic->data[IOAPIC_IOREGSEL] = reg;
	return ioapic->data[IOAPIC_IOWIN];
}

static void ioapic_wr(struct ioapic *ioapic, uint32_t reg, uint32_t v)
{
	ioapic->data[IOAPIC_IOREGSEL] = reg;
	ioapic->data[IOAPIC_IOWIN] = v;
}

void ioapic_init(uint8_t id)
{
	struct ioapic *ioapic = &ioapic0; /* XXX: use id */
	ioapic->data = vmap(0xFEC00000 + id * 0x100, 4096); /* XXX: use addr from ACPI */
	printf("ioapic id: 0x%lx\n", ioapic_rd(ioapic, IOAPIC_REG_ID));
	printf("ioapic version: 0x%lx\n", ioapic_rd(ioapic, IOAPIC_REG_VER));
	ioapic_enable_int(id, ISA_IRQ_PIT);
	ioapic_enable_int(id, ISA_IRQ_KBD);
}

void ioapic_enable_int(uint8_t id, uint8_t intid)
{
	struct ioapic *ioapic = &ioapic0; /* XXX: use id */
	uint32_t reg_base = IOAPIC_REG_REDTBL + intid * 2;
	ioapic_wr(ioapic, reg_base + 0, intid + 32);
	ioapic_wr(ioapic, reg_base + 1, 0xFF << 24); /* XXX: set cpu mask ? */
}
