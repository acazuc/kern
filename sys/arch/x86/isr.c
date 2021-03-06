#include "x86.h"
#include "asm.h"

#include <sys/sched.h>
#include <sys/proc.h>
#include <sys/pcpu.h>
#include <inttypes.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>

static int_handler_t g_interrupt_handlers[256];

static void handle_divide_by_zero(const struct int_ctx *ctx)
{
	panic("divide by zero @ 0x%08" PRIx32 "\n", ctx->trapframe.eip);
}

static void handle_debug(const struct int_ctx *ctx)
{
	uint32_t dr6 = getdr6();
	panic("debug @ 0x%08" PRIx32 " (%08" PRIx32 ")\n", ctx->trapframe.eip, dr6);
}

static void handle_nmi(const struct int_ctx *ctx)
{
	uint8_t scpa = inb(0x92);
	uint8_t scpb = inb(0x61);
	panic("non maskable interrupt @ 0x%08" PRIx32 " (%02" PRIx8 ", %02" PRIx8 ")\n", ctx->trapframe.eip, scpa, scpb);
}

static void handle_breakpoint(const struct int_ctx *ctx)
{
	panic("breakpoint @ 0x%08" PRIx32 "\n", ctx->trapframe.eip);
}

static void handle_overflow(const struct int_ctx *ctx)
{
	panic("overflow @ 0x%08" PRIx32 "\n", ctx->trapframe.eip);
}

static void handle_bound_range_exceeded(const struct int_ctx *ctx)
{
	panic("bound range exceeded @ 0x%08" PRIx32 "\n", ctx->trapframe.eip);
}

static void handle_invalid_opcode(const struct int_ctx *ctx)
{
	panic("invalid opcode @ 0x%08" PRIx32 "\n", ctx->trapframe.eip);
}

static void handle_device_not_available(const struct int_ctx *ctx)
{
	panic("device not available @ 0x%08" PRIx32 "\n", ctx->trapframe.eip);
}

static void handle_double_fault(const struct int_ctx *ctx)
{
	panic("double fault: 0x%" PRIx32 " @ 0x%08" PRIx32 "\n", ctx->err, ctx->trapframe.eip);
}

static void handle_coprocessor_segment_overrun(const struct int_ctx *ctx)
{
	panic("coprocessor segment overrun @ 0x%08" PRIx32 "\n", ctx->trapframe.eip);
}

static void handle_invalid_tss(const struct int_ctx *ctx)
{
	panic("invalid tss: 0x%" PRIx32 " @ 0x%08" PRIx32 "\n", ctx->err, ctx->trapframe.eip);
}

static void handle_segment_not_present(const struct int_ctx *ctx)
{
	panic("segment not present: 0x%" PRIx32 " @ 0x%08" PRIx32 "\n", ctx->err, ctx->trapframe.eip);
}

static void handle_stack_segment_fault(const struct int_ctx *ctx)
{
	panic("stack segment fault: 0x%" PRIx32 " @ 0x%08" PRIx32 "\n", ctx->err, ctx->trapframe.eip);
}

static void handle_general_protection_fault(const struct int_ctx *ctx)
{
	panic("general protection fault: 0x%" PRIx32 " @ 0x%08" PRIx32 "\n", ctx->err, ctx->trapframe.eip);
}

static void handle_page_fault(const struct int_ctx *ctx)
{
	uint32_t page_addr = getcr2();
#if 0
	printf("page fault addr 0x%08" PRIx32 ": 0x%08" PRIx32 " @ 0x%08" PRIx32 " (%" PRIx32 ")\n", page_addr, ctx->err, ctx->trapframe.eip, ctx->trapframe.esp);
#endif
	if (!(ctx->err & 1))
	{
		paging_alloc(page_addr);
		return;
	}
	panic("page protection violation addr 0x%08" PRIx32 ": 0x%08" PRIx32 " @ 0x%08" PRIx32 "\n", page_addr, ctx->err, ctx->trapframe.eip);
}

static void handle_x87_fpe(const struct int_ctx *ctx)
{
	panic("x87 fpe @ 0x%08" PRIx32 "\n", ctx->trapframe.eip);
}

static void handle_alignment_check(const struct int_ctx *ctx)
{
	panic("alignment check @ 0x%08" PRIx32 "\n", ctx->trapframe.eip);
}

static void handle_machine_check(const struct int_ctx *ctx)
{
	panic("machine check @ 0x%08" PRIx32 "\n", ctx->trapframe.eip);
}

static void handle_simd_fpe(const struct int_ctx *ctx)
{
	panic("simd fpe @ 0x%08" PRIx32 "\n", ctx->trapframe.eip);
}

static void handle_virtualization_exception(const struct int_ctx *ctx)
{
	panic("virtualization exception @ 0x%08" PRIx32 "\n", ctx->trapframe.eip);
}

static void handle_control_protection_exception(const struct int_ctx *ctx)
{
	panic("control protection exception @ 0x%08" PRIx32 "\n", ctx->trapframe.eip);
}

static void handle_hypervisor_injection_exception(const struct int_ctx *ctx)
{
	panic("hypervisor injection exception @ 0x%08" PRIx32 "\n", ctx->trapframe.eip);
}

static void handle_vmm_communication_exception(const struct int_ctx *ctx)
{
	panic("vmm communication exception @ 0x%08" PRIx32 "\n", ctx->trapframe.eip);
}

static void handle_security_exception(const struct int_ctx *ctx)
{
	panic("security exception @ 0x%08" PRIx32 "\n", ctx->trapframe.eip);
}

static void handle_syscall(const struct int_ctx *ctx)
{
#if 0
	printf("syscall %" PRId32 " @ 0x%08" PRIx32 " (%" PRIx32 ")\n", ctx->trapframe.eax, ctx->trapframe.eip, ctx->trapframe.esp);
#endif
	call_sys(ctx);
}

void handle_interrupt(uint32_t id, struct int_ctx *ctx)
{
	/*
	 * disable context switching on exception
	 *
	 * exceptions can be thrown while being in kernel mode
	 * and we don't want to bother with nested interrupt trapframe handling
	 */
	if (id >= 0x20 && curthread)
		memcpy(&curthread->tf, &ctx->trapframe, sizeof(ctx->trapframe));
	if (id >= 256)
		panic("invalid interrupt id: 0x%08" PRIx32 " @ 0x%08" PRIx32 " (err: 0x%08" PRIx32 ")\n", id, ctx->trapframe.eip, ctx->err);
	if (!g_interrupt_handlers[id])
		panic("unhandled interrupt 0x%02" PRIx32 " @ 0x%08" PRIx32 " (err: 0x%08" PRIx32 ")\n", id, ctx->trapframe.eip, ctx->err);
	g_interrupt_handlers[id](ctx);
	if (id >= 0x20)
	{
		sched_tick();
		memcpy(&ctx->trapframe, &curthread->tf, sizeof(ctx->trapframe));
		tss_set_ptr(&curthread->int_stack[curthread->int_stack_size]);
	}
}

static int_handler_t g_interrupt_handlers[256] =
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

void set_isa_irq_handler(enum isa_irq_id irq_id, int_handler_t handler)
{
	int id = g_isa_irq[irq_id];
	assert(id > 0, "negative ISA IRQ id");
	assert(id < 0x20, "ISA IRQ id >= 0x20");
	assert(!g_interrupt_handlers[0x20 + id], "ISA IRQ handler already set");
	g_interrupt_handlers[0x20 + id] = handler;
}
