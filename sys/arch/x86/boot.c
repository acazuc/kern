#include "dev/pic/pic.h"
#include "dev/pit/pit.h"
#include "dev/vga/vga.h"
#include "dev/com/com.h"
#include "dev/ps2/ps2.h"
#include "dev/ide/ide.h"
#include "dev/tty/tty.h"
#include "dev/pci/pci.h"
#include "dev/rtc/rtc.h"
#include "dev/acpi/acpi.h"
#include "dev/apic/apic.h"
#include "fs/vfs.h"
#include "x86.h"
#include "asm.h"

#include <sys/sched.h>
#include <multiboot.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int g_isa_irq[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

static int has_apic; /* XXX */

void userland(void);

enum cpuid_feature
{
	CPUID_FEAT_ECX_SSE3         = 0,
	CPUID_FEAT_ECX_PCLMUL       = 1,
	CPUID_FEAT_ECX_DTES64       = 2,
	CPUID_FEAT_ECX_MONITOR      = 3,
	CPUID_FEAT_ECX_DS_CPL       = 4,
	CPUID_FEAT_ECX_VMX          = 5,
	CPUID_FEAT_ECX_SMX          = 6,
	CPUID_FEAT_ECX_EST          = 7,
	CPUID_FEAT_ECX_TM2          = 8,
	CPUID_FEAT_ECX_SSSE3        = 9,
	CPUID_FEAT_ECX_CID          = 10,
	CPUID_FEAT_ECX_SDBG         = 11,
	CPUID_FEAT_ECX_FMA          = 12,
	CPUID_FEAT_ECX_CX16         = 13,
	CPUID_FEAT_ECX_XTPR         = 14,
	CPUID_FEAT_ECX_PDCM         = 15,
	CPUID_FEAT_ECX_PCID         = 17,
	CPUID_FEAT_ECX_DCA          = 18,
	CPUID_FEAT_ECX_SSE4_1       = 19,
	CPUID_FEAT_ECX_SSE4_2       = 20,
	CPUID_FEAT_ECX_X2APIC       = 21,
	CPUID_FEAT_ECX_MOVBE        = 22,
	CPUID_FEAT_ECX_POPCNT       = 23,
	CPUID_FEAT_ECX_TSC          = 24,
	CPUID_FEAT_ECX_AES          = 25,
	CPUID_FEAT_ECX_XSAVE        = 26,
	CPUID_FEAT_ECX_OSXSAVE      = 27,
	CPUID_FEAT_ECX_AVX          = 28,
	CPUID_FEAT_ECX_F16C         = 29,
	CPUID_FEAT_ECX_RDRAND       = 30,
	CPUID_FEAT_ECX_HYPERVISOR   = 31,

	CPUID_FEAT_EDX_FPU          = 0,
	CPUID_FEAT_EDX_VME          = 1,
	CPUID_FEAT_EDX_DE           = 2,
	CPUID_FEAT_EDX_PSE          = 3,
	CPUID_FEAT_EDX_TSC          = 4,
	CPUID_FEAT_EDX_MSR          = 5,
	CPUID_FEAT_EDX_PAE          = 6,
	CPUID_FEAT_EDX_MCE          = 7,
	CPUID_FEAT_EDX_CX8          = 8,
	CPUID_FEAT_EDX_APIC         = 9,
	CPUID_FEAT_EDX_SEP          = 11,
	CPUID_FEAT_EDX_MTRR         = 12,
	CPUID_FEAT_EDX_PGE          = 13,
	CPUID_FEAT_EDX_MCA          = 14,
	CPUID_FEAT_EDX_CMOV         = 15,
	CPUID_FEAT_EDX_PAT          = 16,
	CPUID_FEAT_EDX_PSE36        = 17,
	CPUID_FEAT_EDX_PSN          = 18,
	CPUID_FEAT_EDX_CLFLUSH      = 19,
	CPUID_FEAT_EDX_DS           = 21,
	CPUID_FEAT_EDX_ACPI         = 22,
	CPUID_FEAT_EDX_MMX          = 23,
	CPUID_FEAT_EDX_FXSR         = 24,
	CPUID_FEAT_EDX_SSE          = 25,
	CPUID_FEAT_EDX_SSE2         = 26,
	CPUID_FEAT_EDX_SS           = 27,
	CPUID_FEAT_EDX_HTT          = 28,
	CPUID_FEAT_EDX_TM           = 29,
	CPUID_FEAT_EDX_IA64         = 30,
	CPUID_FEAT_EDX_PBE          = 31
};

static const char *cpuid_feature_ecx_str[32] =
{
	"sse3",
	"pclmul",
	"dtes64",
	"monitor",
	"ds_cpl",
	"vmx",
	"smx",
	"est",
	"tm2",
	"ssse3",
	"cid",
	"sdbg",
	"fma",
	"cx16",
	"xtpr",
	"pdcm",
	"",
	"pcid",
	"dca",
	"sse4_1",
	"sse4_2",
	"x2apic",
	"movbe",
	"popcnt",
	"tsc",
	"aes",
	"xsave",
	"osxsave",
	"avx",
	"f16c",
	"rdrand",
	"hypervisor"
};

static const char *cpuid_feature_edx_str[32] =
{
	"fpu",
	"vme",
	"de",
	"pse",
	"tsc",
	"msr",
	"pae",
	"mce",
	"cx8",
	"apic",
	"",
	"sep",
	"mtrr",
	"pge",
	"mca",
	"cmov",
	"pat",
	"pse36",
	"psn",
	"clflush",
	"",
	"ds",
	"acpi",
	"mmx",
	"fxsr",
	"sse",
	"sse2",
	"ss",
	"htt",
	"tm",
	"ia64",
	"p8e"
};

static void print_multiboot(struct multiboot_info *mb_info)
{
	printf("mb_info: %p\n", mb_info);
	printf("multiboot flags: %08x\n", mb_info->flags);
	if (mb_info->flags & MULTIBOOT_INFO_CMDLINE)
		printf("cmdline: %p\n", (char*)mb_info->cmdline);
	if (mb_info->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME)
		printf("bootloader: %p\n", (char*)mb_info->boot_loader_name);
	printf("BIOS mem: %08x - %08x\n", mb_info->mem_lower, mb_info->mem_upper);
	uint64_t total_mem = 0;
	if (mb_info->flags & MULTIBOOT_INFO_MEM_MAP)
	{
		for (size_t i = 0; i < mb_info->mmap_length; i += sizeof(multiboot_memory_map_t))
		{
			multiboot_memory_map_t *mmap = (multiboot_memory_map_t*)(mb_info->mmap_addr + i);
			printf("mmap %p: %016llx @ %016llx (%08x)\n", mmap, mmap->len, mmap->addr, mmap->type);
			if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
				total_mem += mmap->len;
		}
	}
	printf("total usable memory: %llu MB\n", total_mem / 1024 / 1024);
	if (mb_info->flags & MULTIBOOT_INFO_DRIVE_INFO)
	{
		for (size_t i = 0; i < mb_info->drives_length;)
		{
			multiboot_drive_info_t *drive = (multiboot_drive_info_t*)(mb_info->drives_addr + i);
			printf("number: %hhx, mode: %hhx, cylinders: %hx, heads: %hhx, sectors: %hhx\n", drive->number, drive->mode, drive->cylinders, drive->heads, drive->sectors);
			i += drive->size;
		}
	}
}

static void print_cpuid(void)
{
	uint32_t eax, ebx, ecx, edx;
	char vendor[13];
	uint32_t max_cpuid;
	__cpuid(0, eax, ebx, ecx, edx);
	max_cpuid = eax;
	vendor[0x0] = ebx >> 0;
	vendor[0x1] = ebx >> 8;
	vendor[0x2] = ebx >> 16;
	vendor[0x3] = ebx >> 24;
	vendor[0x4] = edx >> 0;
	vendor[0x5] = edx >> 8;
	vendor[0x6] = edx >> 16;
	vendor[0x7] = edx >> 24;
	vendor[0x8] = ecx >> 0;
	vendor[0x9] = ecx >> 8;
	vendor[0xA] = ecx >> 16;
	vendor[0xB] = ecx >> 24;
	vendor[0xC] = 0;
	printf("vendor: %s\n", vendor);
	if (max_cpuid >= 1)
	{
		__cpuid(1, eax, ebx, ecx, edx);
		uint8_t stepping, model, family, type, ext_model, ext_family;
		stepping =   (eax >> 0x00) & 0xF;
		model =      (eax >> 0x04) & 0xF;
		family =     (eax >> 0x08) & 0xF;
		type =       (eax >> 0x0C) & 0x3;
		ext_model =  (eax >> 0x10) & 0xF;
		ext_family = (eax >> 0x14) & 0xFF;
		printf("stepping: %01x, model: %01x, family: %01x, type: %01x, ext_model: %01x, ext_family: %02x\n", stepping, model, family, type, ext_model, ext_family);
		uint8_t brand, clflush_size, addressable_ids, apic_id;
		brand =           (ebx >> 0x0) & 0xFF;
		clflush_size =    (ebx >> 0x4) & 0xFF;
		addressable_ids = (ebx >> 0x8) & 0xFF;
		apic_id =         (ebx >> 0xC) & 0xFF;
		printf("brand: %02x, clflush_size: %02x, addressable_ids: %02x, apic_id: %02x\n", brand, clflush_size, addressable_ids, apic_id);
		printf("features: ");
		int first = 1;
		for (size_t i = 0; i < 32; ++i)
		{
			if (!(edx & (1 << i)))
				continue;
			if (first)
				first = 0;
			else
				printf(", ");
			printf("%s", cpuid_feature_edx_str[i]);
		}
		for (size_t i = 0; i < 32; ++i)
		{
			if (!(ecx & (1 << i)))
				continue;
			if (first)
				first = 0;
			else
				printf(", ");
			printf("%s", cpuid_feature_ecx_str[i]);
		}
		printf("\n");
		/* XXX: more decoding */
	}
}

static uint32_t mb_get_memory_map_size(struct multiboot_info *mb_info)
{
	if (!(mb_info->flags & MULTIBOOT_INFO_MEM_MAP))
		return 0;
	for (size_t i = 0; i < mb_info->mmap_length; i += sizeof(multiboot_memory_map_t))
	{
		multiboot_memory_map_t *mmap = (multiboot_memory_map_t*)(mb_info->mmap_addr + i);
		if (mmap->type != MULTIBOOT_MEMORY_AVAILABLE)
			continue;
		if (mmap->addr != 0x100000)
			continue;
		return mmap->len;
	}
	return 0;
}

static struct tty *ttys[64];

static void tty_init(void)
{
	for (uint32_t i = 0; i < sizeof(ttys) / sizeof(*ttys); ++i)
	{
		char name[16];
		snprintf(name, sizeof(name), "tty%" PRIu32, i);
		int res = tty_create_vga(name, i, &ttys[i]);
		if (res)
			printf("can't create %s: %s", name, strerror(res));
	}
	curtty = ttys[0];
}

static void idle_loop()
{
	while (1)
	{
		sti();
		hlt();
	}
}

void boot(struct multiboot_info *mb_info)
{
	cli();
	alloc_init();
	vga_init();
	printf("\n");
	printf("x86 boot @ %p\n", boot);
	print_multiboot(mb_info);
	print_cpuid();
	uint32_t mem_size = mb_get_memory_map_size(mb_info);
	assert(mem_size, "can't get memory map\n");
	assert(mem_size >= 0x1000000, "can't get 16MB of memory\n");
	paging_init(0x1000000, mem_size - 0x1000000);
	gdt_init();
	idt_init();
	pic_init(0x20, 0x28);
	vfs_init();
	tty_init();
	*(uint32_t*)(0xFFFFF000) = 0; /* remove identity paging at 0x00000000 */
	pci_init();
	acpi_init();
	has_apic = 1;
	if (has_apic)
	{
		ioapic_init(0);
		lapic_init();
	}
	pit_init();
	rtc_init();
	com_init();
	ps2_init();
	ide_init();
	sched_init();
	struct thread *idle_thread = kproc_create("idle", idle_loop);
	idle_thread->pri = 255;
	sched_add(idle_thread);
	idle_thread->state = THREAD_RUNNING;
	curthread = idle_thread;
	curproc = curthread->proc;
	struct thread *t1, *t2;
	{
		struct thread *thread = uproc_create("init", userland);
		vmm_dup(thread->proc->vmm_ctx, NULL, 0x400000, 0x4000); /* XXX remove */
		sched_add(thread);
		t1 = thread;
	}
	{
		struct thread *thread = uproc_create("init", userland);
		vmm_dup(thread->proc->vmm_ctx, NULL, 0x400000, 0x4000); /* XXX remove */
		sched_add(thread);
		t2 = thread;
	}
	{
		struct file *file;
		struct fs_node *elf;
		assert(!vfs_getnode(NULL, "/bin/sh", &elf), "can't open /bin/sh");
		file = malloc(sizeof(*file), 0);
		file->op = elf->fop;
		file->node = elf;
		file->off = 0;
		struct thread *elf_thread = elf_createproc(file);
	}
	/* added after init to avoid buggy scheduling on page fault interrupt */
	sched_run(t1);
	sched_run(t2);
	idle_loop();
	panic("post idle loop\n");

#if 0
	while (1)
	{
		struct timespec pit_ts;
		struct timespec rtc_ts;
		pit_gettime(&pit_ts);
		rtc_gettime(&rtc_ts);
		int64_t pit_t = pit_ts.tv_sec * 1000000000LL + pit_ts.tv_nsec;
		int64_t rtc_t = rtc_ts.tv_sec * 1000000000LL + rtc_ts.tv_nsec;
		//printf("diff: %lld (%010lld / %010lld)\n", (pit_t - rtc_t) / 1000, pit_t, rtc_t);
	}
#endif

#if 0
	uint8_t buf[512];
	ide_read_sectors(0, 1, 0, (uint8_t*)buf);
	for (size_t i = 0; i < 512; ++i)
		printf("%02x, ", buf[i]);
	printf("\n");
#endif
}

void x86_panic(uint32_t *esp, const char *file, const char *line, const char *fn, const char *fmt, ...)
{
	va_list va_arg;

	va_start(va_arg, fmt);
	vprintf(fmt, va_arg);
	printf("%s@%s:%s\n", fn, file, line);
	printf("EAX: %08lx EBX: %08lx ECX: %08lx EDX: %08lx\n", esp[8], esp[5], esp[7], esp[6]);
	printf("ESI: %08lx EDI: %08lx ESP: %08lx EBP: %08lx\n", esp[2], esp[1], esp[4], esp[3]);
	printf("EIP: %08lx\n", esp[0]);

#if 0
	int i = 0;
	uint32_t *ebp = (uint32_t*)esp[3];
	do
	{
		/* XXX: handle iret stack frame */
		printf("stack: %08lx\n", ebp[1]);
		ebp = (uint32_t*)ebp[0];
	} while (ebp[1] >= 0xC0000000 && ++i < 10);
#endif

infl:
	cli();
	hlt();
	goto infl;
}

void enable_isa_irq(enum isa_irq_id id)
{
	if (has_apic)
		ioapic_enable_irq(0, id); /* XXX: valid ioapic id */
	else
		pic_enable_irq(id);
}

void disable_isa_irq(enum isa_irq_id id)
{
	if (has_apic)
		ioapic_disable_irq(0, id); /* XXX: valid ioapic id */
	else
		pic_disable_irq(id);
}

void isa_eoi(enum isa_irq_id id)
{
	if (has_apic)
		lapic_eoi(id);
	else
		pic_eoi(id);
}

static void init_trapframe(struct thread *thread)
{
	thread->trapframe.eax = 0;
	thread->trapframe.ebx = 0;
	thread->trapframe.ecx = 0;
	thread->trapframe.edx = 0;
	thread->trapframe.esi = 0;
	thread->trapframe.edi = 0;
	thread->trapframe.esp = (uint32_t)&thread->stack[thread->stack_size];
	thread->trapframe.ebp = (uint32_t)&thread->stack[thread->stack_size];
	thread->trapframe.eip = (uint32_t)thread->proc->entrypoint;
}

void init_trapframe_kern(struct thread *thread)
{
	init_trapframe(thread);
	thread->trapframe.cs = 0x08;
	thread->trapframe.ds = 0x10;
	thread->trapframe.es = 0x10;
	thread->trapframe.fs = 0x10;
	thread->trapframe.gs = 0x10;
	thread->trapframe.ss = 0x10;
	thread->trapframe.ef = getef() | (1 << 9); /* IF */
}

void init_trapframe_user(struct thread *thread)
{
	init_trapframe(thread);
	thread->trapframe.cs = 0x1B;
	thread->trapframe.ds = 0x23;
	thread->trapframe.es = 0x23;
	thread->trapframe.fs = 0x23;
	thread->trapframe.gs = 0x23;
	thread->trapframe.ss = 0x23;
	thread->trapframe.ef = getef() | (1 << 9); /* IF */
}
