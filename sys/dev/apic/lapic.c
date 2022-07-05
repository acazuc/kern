#include "apic.h"
#include "arch/x86/asm.h"
#include "arch/arch.h"

#include <stdio.h>

/*
 * for software intended to run on Pentium processors, system
 * software should explicitly not map the APIC register space
 * to regular system memory. Doing so can result in an invalid
 * opcode exception (#UD) being generated or unpredictable execution.
 */

#define LAPIC_REG_ID        0x20
#define LAPIC_REG_VERSION   0x30
#define LAPIC_REG_TPR       0x80
#define LAPIC_REG_APR       0x90
#define LAPIC_REG_PPR       0xA0
#define LAPIC_REG_EOI       0xB0
#define LAPIC_REG_RRD       0xC0
#define LAPIC_REG_LOG_DST   0xD0
#define LAPIC_REG_DST_FMT   0xE0
#define LAPIC_REG_SPUR_IV   0xF0
#define LAPIC_REG_ISR       0x100 /* 8 * 0x10 bytes */
#define LAPIC_REG_TMR       0x180 /* 8 * 0x10 bytes */
#define LAPIC_REG_IRR       0x200 /* 8 * 0x10 bytes */
#define LAPIC_REG_ERR_STT   0x280
#define LAPIC_REG_CMCI      0x2F0
#define LAPIC_REG_ICR       0x300 /* 2 * 0x10 bytes */
#define LAPIC_REG_LVT_TMR   0x320
#define LAPIC_REG_LVT_TSR   0x330
#define LAPIC_REG_LVT_PMC   0x340
#define LAPIC_REG_LVT_LINT0 0x350
#define LAPIC_REG_LVT_LINT1 0x360
#define LAPIC_REG_LVT_LVTE  0x370
#define LAPIC_REG_INIT_CNT  0x380
#define LAPIC_REG_CUR_CNT   0x390
#define LAPIC_REG_DIV_CONF  0x3E0

#define IA32_APIC_BASE 0x1B

static uint8_t volatile *g_addr;

static inline void lapic_set(uint32_t reg, uint32_t v)
{
	*(uint32_t volatile*)&g_addr[reg] = v;
}

static uint32_t lapic_get(uint32_t reg)
{
	return *(uint32_t volatile*)&g_addr[reg];
}

void lapic_init()
{
	uint32_t lo;
	uint32_t hi;
	rdmsr(IA32_APIC_BASE, &hi, &lo);
	printf("lo: %lx, hi: %lx\n", lo, hi);
	g_addr = vmap(0xFEE00000, 4096);
	printf("spur iv: %lx\n", lapic_get(LAPIC_REG_SPUR_IV));
	lapic_set(LAPIC_REG_SPUR_IV, lapic_get(LAPIC_REG_SPUR_IV) | 0x100);
	printf("spur iv: %lx\n", lapic_get(LAPIC_REG_SPUR_IV));
}

void lapic_eoi()
{
	lapic_set(LAPIC_REG_EOI, 0);
}
