#include "x86.h"

typedef struct tss_entry
{
	uint32_t prev_tss;
	uint32_t esp0;
	uint32_t ss0;
	uint32_t esp1;
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	uint32_t ldt;
	uint16_t trap;
	uint16_t iomap_base;
} __attribute__((packed)) tss_entry_t;

typedef struct gdt_entry
{
	uint32_t base;
	uint32_t limit;
	uint8_t type;
	uint8_t flags;
} gdt_entry_t;

typedef struct gtdr
{
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) gdtr_t;

static uint8_t g_tss_stack[4096 * 4]; /* per-process stack */
static tss_entry_t g_tss;
static gdtr_t g_gdtr;

static const gdt_entry_t g_gdt_entries[] =
{
	{.base = 0               , .limit = 0            , .type = 0x00, .flags = 0x40}, /* NULL */
	{.base = 0               , .limit = 0xFFFFFFFF   , .type = 0x9A, .flags = 0x40}, /* code */
	{.base = 0               , .limit = 0xFFFFFFFF   , .type = 0x92, .flags = 0x40}, /* data */
	{.base = 0               , .limit = 0xFFFFFFFF   , .type = 0xFA, .flags = 0x40}, /* user code */
	{.base = 0               , .limit = 0xFFFFFFFF   , .type = 0xF2, .flags = 0x40}, /* user data */
	{.base = (uint32_t)&g_tss, .limit = sizeof(g_tss), .type = 0xE9, .flags = 0x00}, /* tss */
};

static uint8_t gdt_data[8 * sizeof(g_gdt_entries) / sizeof(*g_gdt_entries)];

static void encode_entry(uint8_t *target, gdt_entry_t source)
{
	if (source.limit > 65536)
	{
		source.limit = source.limit >> 12;
		target[6] = source.flags | 0x80;
	}
	else
	{
		target[6] = source.flags;
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
	memset(&g_tss, 0, sizeof(g_tss));
	g_tss.ss0 = 0x10;
	g_tss.esp0 = (uint32_t)&g_tss_stack[sizeof(g_tss_stack)];
	__asm__ volatile ("lgdt %0" : : "m"(g_gdtr));
	__asm__ volatile ("mov $0x28, %%ax; ltr %%ax" ::: "eax");
	reload_segments();
}
