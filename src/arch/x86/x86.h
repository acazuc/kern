#ifndef X86_H
#define X86_H

#include "arch/arch.h"
#include "sys/std.h"

#include <stdint.h>
#include <stddef.h>

void idt_init(void);
void reload_segments(void);
void gdt_init(void);
void paging_init(uint32_t addr, uint32_t size);
void paging_alloc(uint32_t addr);
void paging_vmalloc(uint32_t addr, uint32_t size);
void paging_dumpinfo();
void x86_panic(uint32_t *esp, const char *file, const char *line, const char *fn, const char *fmt, ...)  __attribute__((format(printf, 5, 6)));

#define panic(...) \
do \
{ \
	uint32_t esp; \
	__asm__ volatile ("push $2; pusha; call 1f; call 2f; 1: pop %%eax; push %%eax; ret; 2: push %%eax; mov %%esp, %0" : "=a"(esp)); \
	x86_panic((uint32_t*)esp, __FILE__, QUOTEME(__LINE__), __func__, __VA_ARGS__); \
} while (0)

#endif
