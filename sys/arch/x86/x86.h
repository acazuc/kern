#ifndef X86_H
#define X86_H

#include <sys/std.h>
#include <stdint.h>
#include <stddef.h>
#include <arch.h>

enum isa_irq_id
{
	ISA_IRQ_PIT,
	ISA_IRQ_KBD,
	ISA_IRQ_CASCADE,
	ISA_IEQ_COM2,
	ISA_IRQ_COM1,
	ISA_IRQ_LPT2,
	ISA_IRQ_FLOPPY,
	ISA_IRQ_LPT1,
	ISA_IRQ_CMOS,
	ISA_IRQ_FREE1,
	ISA_IRQ_FREE2,
	ISA_IRQ_FREE3,
	ISA_IRQ_MOUSE,
	ISA_IRQ_FPU,
	ISA_IRQ_ATA1,
	ISA_IRQ_ATA2,
};

struct int_ctx
{
	struct trapframe trapframe;
	uint32_t err;
};

typedef void (*int_handler_t)(const struct int_ctx*);

void idt_init(void);
void reload_segments(void);
void gdt_init(void);
void paging_init(uint32_t addr, uint32_t size);
void paging_alloc(uint32_t addr);
void paging_dumpinfo(void);
void call_sys(const struct int_ctx *ctx);
void set_isa_irq_handler(enum isa_irq_id irq, int_handler_t handler);
void enable_isa_irq(enum isa_irq_id id);
void disable_isa_irq(enum isa_irq_id id);
void isa_eoi(enum isa_irq_id id);
void tss_set_ptr(void *ptr);

extern int g_isa_irq[16];

#endif
