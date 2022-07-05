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

static inline void insl(uint16_t port, uint32_t *buffer, uint32_t n)
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

#endif
