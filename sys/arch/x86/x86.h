#ifndef X86_H
#define X86_H

#include "arch/arch.h"

#include <sys/std.h>
#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096


void idt_init(void);
void reload_segments(void);
void gdt_init(void);
void paging_init(uint32_t addr, uint32_t size);
void paging_alloc(uint32_t addr);
void paging_dumpinfo(void);
void x86_panic(uint32_t *esp, const char *file, const char *line, const char *fn, const char *fmt, ...)  __attribute__((format(printf, 5, 6)));
void usermode(void (*fn)(void));
uint32_t call_sys(uint32_t *args);
int set_irq_handler(int id, void (*handler)(void));

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
