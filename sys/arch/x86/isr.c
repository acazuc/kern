#include "dev/ps2/ps2.h"
#include "dev/pit/pit.h"
#include "x86.h"
#include "io.h"

#include <stdint.h>
#include <stdio.h>
#include <errno.h>

struct exception_ctx
{
	uint32_t eip;
	uint32_t err;
};

static void (*g_exception_handlers[256])(struct exception_ctx *ctx);

static void handle_divide_by_zero(struct exception_ctx *ctx)
{
	panic("divide by zero @ 0x%08lx\n", ctx->eip);
}

static void handle_debug(struct exception_ctx *ctx)
{
	panic("debug @ 0x%08lx\n", ctx->eip);
}

static void handle_nmi(struct exception_ctx *ctx)
{
	panic("non maskable interrupt @ 0x%08lx\n", ctx->eip);
}

static void handle_breakpoint(struct exception_ctx *ctx)
{
	panic("breakpoint @ 0x%08lx\n", ctx->eip);
}

static void handle_overflow(struct exception_ctx *ctx)
{
	panic("overflow @ 0x%08lx\n", ctx->eip);
}

static void handle_bound_range_exceeded(struct exception_ctx *ctx)
{
	panic("bound range exceeded @ 0x%08lx\n", ctx->eip);
}

static void handle_invalid_opcode(struct exception_ctx *ctx)
{
	panic("invalid opcode @ 0x%08lx\n", ctx->eip);
}

static void handle_device_not_available(struct exception_ctx *ctx)
{
	panic("device not available @ 0x%08lx\n", ctx->eip);
}

static void handle_double_fault(struct exception_ctx *ctx)
{
	panic("double fault: 0x%lx @ 0x%08lx\n", ctx->err, ctx->eip);
}

static void handle_coprocessor_segment_overrun(struct exception_ctx *ctx)
{
	panic("coprocessor segment overrun @ 0x%08lx\n", ctx->eip);
}

static void handle_invalid_tss(struct exception_ctx *ctx)
{
	panic("invalid tss: 0x%lx @ 0x%08lx\n", ctx->err, ctx->eip);
}

static void handle_segment_not_present(struct exception_ctx *ctx)
{
	panic("segment not present: 0x%lx @ 0x%08lx\n", ctx->err, ctx->eip);
}

static void handle_stack_segment_fault(struct exception_ctx *ctx)
{
	panic("stack segment fault: 0x%lx @ 0x%08lx\n", ctx->err, ctx->eip);
}

static void handle_general_protection_fault(struct exception_ctx *ctx)
{
	panic("general protection fault: 0x%lx @ 0x%08lx\n", ctx->err, ctx->eip);
}

static void handle_page_fault(struct exception_ctx *ctx)
{
	uint32_t page_addr;
	__asm__ volatile ("mov %%cr2, %0" : "=a"(page_addr));
	if (!(ctx->err & 1))
	{
		paging_alloc(page_addr);
		return;
	}
	panic("page protection violation addr 0x%08lx: 0x%08lx @ 0x%08lx\n", page_addr, ctx->err, ctx->eip);
}

static void handle_x87_fpe(struct exception_ctx *ctx)
{
	panic("x87 fpe @ 0x%08lx\n", ctx->eip);
}

static void handle_alignment_check(struct exception_ctx *ctx)
{
	panic("alignment check @ 0x%08lx\n", ctx->eip);
}

static void handle_machine_check(struct exception_ctx *ctx)
{
	panic("machine check @ 0x%08lx\n", ctx->eip);
}

static void handle_simd_fpe(struct exception_ctx *ctx)
{
	panic("simd fpe @ 0x%08lx\n", ctx->eip);
}

static void handle_virtualization_exception(struct exception_ctx *ctx)
{
	panic("virtualization exception @ 0x%08lx\n", ctx->eip);
}

static void handle_control_protection_exception(struct exception_ctx *ctx)
{
	panic("control protection exception @ 0x%08lx\n", ctx->eip);
}

static void handle_hypervisor_injection_exception(struct exception_ctx *ctx)
{
	panic("hypervisor injection exception @ 0x%08lx\n", ctx->eip);
}

static void handle_vmm_communication_exception(struct exception_ctx *ctx)
{
	panic("vmm communication exception @ 0x%08lx\n", ctx->eip);
}

static void handle_security_exception(struct exception_ctx *ctx)
{
	panic("security exception @ 0x%08lx\n", ctx->eip);
}

static void handle_syscall(struct exception_ctx *ctx)
{
	uint32_t *args = (uint32_t*)ctx;
	args[0] = call_sys(args + 1);
}

void handle_exception(uint32_t id, struct exception_ctx *ctx)
{
	if (id >= 256)
		panic("invalid exception id: 0x%08lx @ 0x%08lx (err: 0x%08lx)\n", id, ctx->eip, ctx->err);
	if (!g_exception_handlers[id])
		panic("unhandled exception 0x%02lx @ 0x%08lx (err: 0x%08lx)\n", id, ctx->eip, ctx->err);
	g_exception_handlers[id](ctx);
}

static void (*g_exception_handlers[256])(struct exception_ctx*) =
{
	[0x0]  = handle_divide_by_zero,
	[0x1]  = handle_debug,
	[0x2]  = handle_nmi,
	[0x3]  = handle_breakpoint,
	[0x4]  = handle_overflow,
	[0x5]  = handle_bound_range_exceeded,
	[0x6]  = handle_invalid_opcode,
	[0x7]  = handle_device_not_available,
	[0x8]  = handle_double_fault,
	[0x9]  = handle_coprocessor_segment_overrun,
	[0xA]  = handle_invalid_tss,
	[0xB]  = handle_segment_not_present,
	[0xC]  = handle_stack_segment_fault,
	[0xD]  = handle_general_protection_fault,
	[0xE]  = handle_page_fault,
	[0x10] = handle_x87_fpe,
	[0x11] = handle_alignment_check,
	[0x12] = handle_machine_check,
	[0x13] = handle_simd_fpe,
	[0x14] = handle_virtualization_exception,
	[0x15] = handle_control_protection_exception,
	[0x1C] = handle_hypervisor_injection_exception,
	[0x1D] = handle_vmm_communication_exception,
	[0x1E] = handle_security_exception,
	[0x80] = handle_syscall,
};

int set_irq_handler(int id, void (*handler)(void))
{
	if (id < 0 || id >= 0x20 || g_exception_handlers[0x20 + id])
		return EINVAL;
	g_exception_handlers[0x20 + id] = handler;
	return 0;
}
