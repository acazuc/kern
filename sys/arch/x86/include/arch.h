#ifndef ARCH_H
#define ARCH_H

#include <sys/std.h>

#define PAGE_SIZE 4096
#define MAXCPU 256

#define panic(...) \
do \
{ \
	uint32_t esp; \
	__asm__ volatile ("cli; pusha; call 1f; jmp 2f; 1: pop %%eax; push %%eax; ret; 2: push %%eax; mov %%esp, %0" : "=b"(esp) :: "eax"); \
	x86_panic((uint32_t*)esp, __FILE__, QUOTEME(__LINE__), __func__, __VA_ARGS__); \
} while (0)

void x86_panic(uint32_t *esp, const char *file, const char *line, const char *fn, const char *fmt, ...)  __attribute__((format(printf, 5, 6)));

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

#endif
