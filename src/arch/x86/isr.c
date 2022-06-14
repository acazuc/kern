#include "x86.h"

#include "arch/x86/io.h"
#include "sys/std.h"
#include "dev/ps2/ps2.h"
#include "dev/pit/pit.h"

#include <stdint.h>

static void (*g_exception_handlers[256])(uint32_t);

static void handle_divide_by_zero(uint32_t err)
{
	(void)err;
	panic("divide by zero\n");
}

static void handle_debug(uint32_t err)
{
	(void)err;
	panic("debug\n");
}

static void handle_nmi(uint32_t err)
{
	(void)err;
	panic("non maskable interrupt\n");
}

static void handle_breakpoint(uint32_t err)
{
	(void)err;
	panic("breakpoint\n");
}

static void handle_overflow(uint32_t err)
{
	(void)err;
	panic("overflow\n");
}

static void handle_bound_range_exceeded(uint32_t err)
{
	(void)err;
	panic("bound range exceeded\n");
}

static void handle_invalid_opcode(uint32_t err)
{
	(void)err;
	panic("invalid opcode\n");
}

static void handle_page_fault(uint32_t err)
{
	uint32_t page_addr;
	__asm__ volatile ("mov %%cr2, %0" : "=a"(page_addr));
	if (!(err & 1))
	{
		paging_alloc(page_addr);
		return;
	}
	panic("page protection violation @ %08lx: %08lx\n", page_addr, err);
}

static void irq_handler_32(uint32_t err)
{
	(void)err;
	pit_interrupt();
	outb(0x20, 0x20);
}

static void irq_handler_33(uint32_t err)
{
	(void)err;
	ps2_interrupt();
	outb(0x20, 0x20);
}

void handle_exception(uint32_t id, uint32_t err)
{
	if (id >= 256)
		panic("invalid exception id: %08lx (err: %08lx)\n", id, err);
	if (!g_exception_handlers[id])
		panic("unhandled exception %02lx (err: %08lx)\n", id, err);
	g_exception_handlers[id](err);
}

static void (*g_exception_handlers[256])(uint32_t) =
{
	[0x0]  = handle_divide_by_zero,
	[0x1]  = handle_debug,
	[0x2]  = handle_nmi,
	[0x3]  = handle_breakpoint,
	[0x4]  = handle_overflow,
	[0x5]  = handle_bound_range_exceeded,
	[0x6]  = handle_invalid_opcode,
	[0xE]  = handle_page_fault,
	[0x20] = irq_handler_32,
	[0x21] = irq_handler_33,
};
