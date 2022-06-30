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

#endif
