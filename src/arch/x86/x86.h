#ifndef X86_H
#define X86_H

#include "arch/arch.h"
#include "sys/std.h"

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

uint32_t syscall(uint32_t id, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, uint32_t arg6);
static inline int32_t write(int fd, const void *buffer, size_t count)
{
	int32_t ret = (int32_t)syscall(0x4, fd, (uint32_t)buffer, count, 0, 0, 0);
	if (ret < 0)
		ret = -1; /* XXX: errno */
	return ret;
}

#define panic(...) \
do \
{ \
	uint32_t esp; \
	__asm__ volatile ("cli; pusha; call 1f; jmp 2f; 1: pop %%eax; push %%eax; ret; 2: push %%eax; mov %%esp, %0" : "=b"(esp) :: "eax", "esp"); \
	x86_panic((uint32_t*)esp, __FILE__, QUOTEME(__LINE__), __func__, __VA_ARGS__); \
} while (0)

#endif
