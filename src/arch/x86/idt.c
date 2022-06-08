#include "idt.h"

extern void *g_isr_table[];

typedef struct idt_entry
{
	uint16_t isr_low;    /* the lower 16 bits of the ISR's address */
	uint16_t kernel_cs;  /* the GDT segment selector that the CPU will load into CS before calling the ISR */
	uint8_t  reserved;   /* set to zero */
	uint8_t  attributes; /* type and attributes; see the IDT page */
	uint16_t isr_high;   /* the higher 16 bits of the ISR's address */
} __attribute__((packed)) idt_entry_t;

typedef struct idtr
{
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) idtr_t;

static idtr_t g_idtr;
static idt_entry_t g_idt[256];

static void set_descriptor(struct idt_entry *descriptor, void *isr, uint8_t flags)
{
	descriptor->isr_low    = (uint32_t)isr & 0xFFFF;
	descriptor->kernel_cs  = 0x08;
	descriptor->attributes = flags;
	descriptor->isr_high   = (uint32_t)isr >> 16;
	descriptor->reserved   = 0;
}

void idt_init()
{
	for (int i = 0; i < 256; ++i)
		set_descriptor(&g_idt[i], g_isr_table[i], 0x8E);
	g_idtr.base = (uintptr_t)&g_idt[0];
	g_idtr.limit = (uint16_t)sizeof(struct idt_entry) * sizeof(g_idt) / sizeof(*g_idt) - 1;
	__asm__ volatile ("lidt %0" : : "m"(g_idtr));
}
