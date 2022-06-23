#include "x86.h"

#include "arch/x86/io.h"
#include "sys/std.h"
#include "dev/ps2/ps2.h"
#include "dev/pit/pit.h"
#include "sys/errno.h"

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

static void handle_device_not_available(uint32_t err)
{
	(void)err;
	panic("device not available\n");
}

static void handle_double_fault(uint32_t err)
{
	panic("double fault: 0x%lx\n", err);
}

static void handle_coprocessor_segment_overrun(uint32_t err)
{
	(void)err;
	panic("coprocessor segment overrun\n");
}

static void handle_invalid_tss(uint32_t err)
{
	panic("invalid tss: 0x%lx\n", err);
}

static void handle_segment_not_present(uint32_t err)
{
	panic("segment not present: 0x%lx\n", err);
}

static void handle_stack_segment_fault(uint32_t err)
{
	panic("stack segment fault: 0x%lx\n", err);
}

static void handle_general_protection_fault(uint32_t err)
{
	panic("general protection fault: 0x%lx\n", err);
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
	panic("page protection violation @ 0x%08lx: 0x%08lx\n", page_addr, err);
}

static void handle_x87_fpe(uint32_t err)
{
	(void)err;
	panic("x87 fpe\n");
}

static void handle_alignment_check(uint32_t err)
{
	(void)err;
	panic("alignment check\n");
}

static void handle_machine_check(uint32_t err)
{
	(void)err;
	panic("machine check\n");
}

static void handle_simd_fpe(uint32_t err)
{
	(void)err;
	panic("simd fpe\n");
}

static void handle_virtualization_exception(uint32_t err)
{
	(void)err;
	panic("virtualization exception\n");
}

static void handle_control_protection_exception(uint32_t err)
{
	(void)err;
	panic("control protection exception\n");
}

static void handle_hypervisor_injection_exception(uint32_t err)
{
	(void)err;
	panic("hypervisor injection exception\n");
}

static void handle_vmm_communication_exception(uint32_t err)
{
	(void)err;
	panic("vmm communication exception\n");
}

static void handle_security_exception(uint32_t err)
{
	(void)err;
	panic("security exception\n");
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

static void irq_handler_36(uint32_t err)
{
	(void)err;
	printf("com irq\n");
	outb(0x20, 0x20);
}

static void handle_syscall(uint32_t err)
{
	uint32_t *args = (uint32_t*)err;
	uint32_t id = args[1];
	switch (id)
	{
		case 4:
		{
			int fd = args[2];
			void *data = (void*)args[3];
			size_t count = args[4];
			if (fd != 1)
			{
				args[0] = -EBADF;
				break;
			}
			for (size_t i = 0; i < count; ++i)
				printf("%c", ((char*)data)[i]);
			args[0] = count;
			break;
		}
		default:
			args[0] = -ENOSYS;
			break;
	}
}

void handle_exception(uint32_t id, uint32_t err)
{
	if (id >= 256)
		panic("invalid exception id: 0x%08lx (err: 0x%08lx)\n", id, err);
	if (!g_exception_handlers[id])
		panic("unhandled exception 0x%02lx (err: 0x%08lx)\n", id, err);
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
	[0x20] = irq_handler_32,
	[0x21] = irq_handler_33,
	[0x24] = irq_handler_36,
	[0x80] = handle_syscall,
};
