#ifndef X86_H
#define X86_H

#include "arch/arch.h"

#include <sys/std.h>
#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096

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

struct trapframe
{
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	uint32_t esi;
	uint32_t edi;
	uint32_t esp;
	uint32_t ebp;
	uint32_t eip;
	uint32_t cs;
	uint32_t ds;
	uint32_t es;
	uint32_t fs;
	uint32_t gs;
	uint32_t ss;
	uint32_t ef;
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
void x86_panic(uint32_t *esp, const char *file, const char *line, const char *fn, const char *fmt, ...)  __attribute__((format(printf, 5, 6)));
void call_sys(const struct int_ctx *ctx);
void set_isa_irq_handler(enum isa_irq_id irq, int_handler_t handler);
void enable_isa_irq(enum isa_irq_id id);
void disable_isa_irq(enum isa_irq_id id);
void isa_eoi(enum isa_irq_id id);
void tss_set_ptr(void *ptr);

extern int g_isa_irq[16];

#define panic(...) \
do \
{ \
	uint32_t esp; \
	__asm__ volatile ("cli; pusha; call 1f; jmp 2f; 1: pop %%eax; push %%eax; ret; 2: push %%eax; mov %%esp, %0" : "=b"(esp) :: "eax"); \
	x86_panic((uint32_t*)esp, __FILE__, QUOTEME(__LINE__), __func__, __VA_ARGS__); \
} while (0)

#define assert(expression, ...) \
do \
{ \
	if (!(expression)) \
		panic(__VA_ARGS__); \
} while (0)

#endif
