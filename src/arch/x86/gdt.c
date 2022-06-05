#include "gdt.h"

typedef struct gdt_entry
{
	uint32_t base;
	uint32_t limit;
	uint8_t type;
} gdt_entry_t;

typedef struct gtdr
{
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) gdtr_t;

static gdtr_t g_gdtr;

static const gdt_entry_t g_gdt_entries[] =
{
	{.base = 0, .limit = 0         , .type = 0},    /* NULL */
	{.base = 0, .limit = 0xFFFFFFFF, .type = 0x9A}, /* code */
	{.base = 0, .limit = 0xFFFFFFFF, .type = 0x92}, /* data */
};

static uint8_t gdt_data[8 * sizeof(g_gdt_entries) / sizeof(*g_gdt_entries)];

static void encode_entry(uint8_t *target, gdt_entry_t source)
{
	if (source.limit > 65536)
	{
		source.limit = source.limit >> 12;
		target[6] = 0xC0;
	}
	else
	{
		target[6] = 0x40;
	}

	target[0] = source.limit & 0xFF;
	target[1] = (source.limit >> 8) & 0xFF;
	target[2] = source.base & 0xFF;
	target[3] = (source.base >> 8) & 0xFF;
	target[4] = (source.base >> 16) & 0xFF;
	target[5] = source.type;
	target[6] |= (source.limit >> 16) & 0xF;
	target[7] = (source.base >> 24) & 0xFF;
}

void gdt_init()
{
	for (size_t i = 0; i < sizeof(g_gdt_entries) / sizeof(*g_gdt_entries); ++i)
		encode_entry(&gdt_data[8 * i], g_gdt_entries[i]);
	g_gdtr.base = (uintptr_t)&gdt_data[0];
	g_gdtr.limit = (uint16_t)sizeof(gdt_data) - 1;
	__asm__ volatile ("lgdt %0" : : "m"(g_gdtr));
	reload_segments();
}
