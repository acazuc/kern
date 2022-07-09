#ifndef X86_ASM_H
#define X86_ASM_H

#include <stdint.h>

static inline uint8_t inb(uint16_t port)
{
	uint8_t ret;
	__asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

static inline uint32_t inl(uint16_t port)
{
	uint32_t ret;
	__asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

static inline void outb(uint16_t port, uint8_t val)
{
	__asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outl(uint16_t port, uint32_t val)
{
	__asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline void io_wait(void)
{
    outb(0x80, 0);
}

static inline void inla(uint16_t port, uint32_t *buffer, uint32_t n)
{
	for (uint32_t i = 0; i < n; ++i)
		buffer[i] = inl(port);
}

static inline void cli(void)
{
	__asm__ volatile ("cli");
}

static inline void sti(void)
{
	__asm__ volatile ("sti");
}

static inline void hlt(void)
{
	__asm__ volatile ("hlt");
}

static inline void rdmsr(uint32_t msr, uint32_t *h32, uint32_t *l32)
{
	__asm__ volatile ("rdmsr" : "=d"(*h32), "=a"(*l32) : "c"(msr));
}

static inline void wrmsr(uint32_t msr, uint32_t h32, uint32_t l32)
{
	__asm__ volatile ("wrmsr" : : "c"(msr), "d"(h32), "a"(l32));
}

static inline void invlpg(uint32_t vaddr)
{
	__asm__ volatile ("invlpg (%0)" : : "a"(vaddr) : "memory");
}

static inline void repinsw(void *dst, void *src, uint32_t n)
{
	__asm__ volatile ("rep insw" : : "c"(n), "d"(src), "D"(dst));
}

static inline void repoutsw(void *dst, void *src, uint32_t n)
{
	__asm__ volatile ("rep outsw" : : "c"(n), "d"(dst), "D"(src));
}

static inline uint32_t getef(void)
{
	uint32_t ret;
	__asm__ volatile ("pushf; mov (%%esp), %0; add $4, %%esp" : "=a"(ret));
	return ret;
}

static inline uint32_t getdr6(void)
{
	uint32_t ret;
	__asm__ volatile ("mov %%dr6, %0" : "=a"(ret));
	return ret;
}

static inline void setcr3(uint32_t addr)
{
	__asm__ volatile ("mov %0, %%cr3" : : "a"(addr));
}

static inline uint32_t getcr2(void)
{
	uint32_t ret;
	__asm__ volatile ("mov %%cr2, %0" : "=a"(ret));
	return ret;
}

#define __cpuid(n, eax, ebx, ecx, edx) __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(n))

#endif
