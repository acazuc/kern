#include "dev/pic/pic.h"
#include "dev/pit/pit.h"
#include "dev/vga/vga.h"
#include "dev/com/com.h"
#include "dev/ps2/ps2.h"
#include "dev/ide/ide.h"
#include "dev/pci/pci.h"
#include "dev/rtc/rtc.h"
#include "dev/acpi/acpi.h"
#include "dev/apic/apic.h"
#include "x86.h"
#include "asm.h"
#include "msr.h"

#include <sys/sched.h>
#include <multiboot.h>
#include <sys/pcpu.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <inttypes.h>
#include <sys/vmm.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <vfs.h>
#include <tty.h>

int g_isa_irq[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

struct pcpu pcpu[MAXCPU];
uint8_t *ap_stacks[MAXCPU];

static int has_apic; /* XXX */

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
	printf("multiboot flags: %08" PRIx32 "\n", mb_info->flags);
	if (mb_info->flags & MULTIBOOT_INFO_CMDLINE)
		printf("cmdline: %p\n", (char*)mb_info->cmdline);
	if (mb_info->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME)
		printf("bootloader: %p\n", (char*)mb_info->boot_loader_name);
	printf("BIOS mem: %08" PRIx32 " - %08" PRIx32 "\n", mb_info->mem_lower, mb_info->mem_upper);
	uint64_t total_mem = 0;
	if (mb_info->flags & MULTIBOOT_INFO_MEM_MAP)
	{
		for (size_t i = 0; i < mb_info->mmap_length; i += sizeof(multiboot_memory_map_t))
		{
			multiboot_memory_map_t *mmap = (multiboot_memory_map_t*)(mb_info->mmap_addr + i);
			printf("mmap %p: %016" PRIx64 " @ %016" PRIx64 " (%08" PRIx32 ")\n", mmap, mmap->len, mmap->addr, mmap->type);
			if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
				total_mem += mmap->len;
		}
	}
	printf("total usable memory: %" PRIu64 " MB\n", total_mem / 1024 / 1024);
	if (mb_info->flags & MULTIBOOT_INFO_DRIVE_INFO)
	{
		for (size_t i = 0; i < mb_info->drives_length;)
		{
			multiboot_drive_info_t *drive = (multiboot_drive_info_t*)(mb_info->drives_addr + i);
			printf("number: %" PRIx8 ", mode: %" PRIx8 ", cylinders: %" PRIx16 ", heads: %" PRIx8 ", sectors: %" PRIx8 "\n", drive->number, drive->mode, drive->cylinders, drive->heads, drive->sectors);
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
		printf("stepping: %01" PRIx8 ", model: %01" PRIx8 ", family: %01" PRIx8 ", type: %01" PRIx8 ", ext_model: %01" PRIx8 ", ext_family: %02" PRIx8 "\n", stepping, model, family, type, ext_model, ext_family);
		uint8_t brand, clflush_size, addressable_ids, apic_id;
		brand =           (ebx >>  0) & 0xFF;
		clflush_size =    (ebx >>  8) & 0xFF;
		addressable_ids = (ebx >> 16) & 0xFF;
		apic_id =         (ebx >> 24) & 0xFF;
		printf("brand: %02" PRIx8 ", clflush_size: %02" PRIx8 ", addressable_ids: %02" PRIx8 ", apic_id: %02" PRIx8 "\n", brand, clflush_size, addressable_ids, apic_id);
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
		int res = vga_mktty(name, i, &ttys[i]); /* XXX: use rgb tty if any */
		if (res)
			printf("can't create %s: %s", name, strerror(res));
	}
	curtty = ttys[0];
}

static void idle_loop(void)
{
	while (1)
	{
		sti();
		hlt();
	}
}

void ap_trampoline();

volatile int ap_startup_done;
volatile size_t ap_running = 1;

void ap_startup(void *p)
{
	uint32_t cpuid = curcpu();
	printf("running ap %" PRIu32 "\n", cpuid);
	idle_loop();
	char name[64];
	snprintf(name, sizeof(name), "idle%" PRIu32, cpuid);
	const char *argv[] = {"idle", NULL};
	const char *envp[] = {NULL};
	idlethread = kproc_create(name, idle_loop, argv, envp);
	idlethread->pri = 255;
	CPUMASK_CLEAR(&idlethread->affinity);
	CPUMASK_SET(&idlethread->affinity, cpuid, 1);
	sched_add(idlethread);
	idlethread->state = THREAD_RUNNING;
	curthread = idlethread;
	idle_loop();
	panic("post idle loop\n");
	while(1);
}

static void start_ap(void)
{
	uint8_t lapic_id = curcpu();
	size_t numcores = 4;
	uint8_t lapics[] = {0, 1, 2, 3}; /* XXX */

	void *dst = vmap(0x8000, 4096);
	memcpy(dst, &ap_trampoline, 4096);
	uint8_t *apic_table = (uint8_t*)dst + 0x60;

	uint32_t msrl, msrh;
	rdmsr(IA32_APIC_BASE, &msrh, &msrl);
	volatile uint32_t *lapic_ptr = vmap(msrl & 0xFFFFF000, 4096);
	assert(lapic_ptr, "can't map lapic ptr\n");

	for (size_t i = 0; i < numcores; ++i)
	{
		if (lapics[i] == lapic_id)
			continue;
		uint8_t **stackp = &pcpu[lapics[i]].stack;
		size_t *stack_size = &pcpu[lapics[i]].stack_size;
		*stack_size = 1024 * 16;
		*stackp = vmalloc(*stack_size);
		assert(*stackp, "can't malloc ap stack");
		memset(*stackp, 0, *stack_size); /* must be mapped */
		*stackp += *stack_size;
		ap_stacks[lapics[i]] = *stackp;

		/* INIT IPI */
		lapic_ptr[0xA0] = 0;
		lapic_ptr[0xC4] = (lapic_ptr[0xC4] & 0x00FFFFFF) | (i << 24);
		lapic_ptr[0xC0] = (lapic_ptr[0xC0] & 0xFFF00000) | 0x00C500;
		do
		{
			__asm__ volatile ("pause" : : : "memory");
		} while (lapic_ptr[0xC0] & (1 << 12));

		lapic_ptr[0xC4] = (lapic_ptr[0xC4] & 0x00FFFFFF) | (i << 24);
		lapic_ptr[0xC0] = (lapic_ptr[0xC0] & 0xFFF00000) | 0x008500;
		do
		{
			__asm__ volatile ("pause" : : : "memory");
		} while (lapic_ptr[0xC0] & (1 << 12));

		/* XXX wait 10 ms */
		/* XXX send startup only if not 82489DX */
		for (size_t j = 0; j < 2; ++j)
		{
			/* STARTUP IPI */
			lapic_ptr[0xA0] = 0;
			lapic_ptr[0xC4] = (lapic_ptr[0xC4] & 0x00FFFFFF) | (i << 24);
			lapic_ptr[0xC0] = (lapic_ptr[0xC0] & 0xFFF0F800) | 0x000608;
			/* XXX wait 200 us */
			do
			{
				__asm__ volatile ("pause" : : : "memory");
			} while (lapic_ptr[0xC0] & (1 << 12));
		}
	}
	vunmap((void*)lapic_ptr, 4096);
	vunmap(dst, 4096);
	ap_startup_done = 1; /* release all the AP */
	while (ap_running < numcores) {}
}

void kernel_main(struct multiboot_info *mb_info)
{
	uint32_t mem_size = mb_get_memory_map_size(mb_info);
	assert(mem_size, "can't get memory map\n");
	assert(mem_size >= 0x1000000, "can't get 16MB of memory\n");
	paging_init(0x1000000, mem_size - 0x1000000);
	alloc_init();
	if (mb_info->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO)
	{
		switch (mb_info->framebuffer_type)
		{
			case MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT:
				vga_init(0, mb_info->framebuffer_addr, mb_info->framebuffer_width, mb_info->framebuffer_height, mb_info->framebuffer_pitch, 0);
				break;
			case MULTIBOOT_FRAMEBUFFER_TYPE_RGB:
				vga_init(1, mb_info->framebuffer_addr, mb_info->framebuffer_width, mb_info->framebuffer_height, mb_info->framebuffer_pitch, mb_info->framebuffer_bpp); /* XXX: use mask & fields from mb_info ? */
				break;
			default:
				panic("can't init vga\n");
				return;
		}
	}
	else
	{
		vga_init(0, 0xB8000, 80, 25, 80 * 2, 0);
	}
	gdt_init();
	idt_init();
	pic_init(0x20, 0x28);
	com_init();
	vfs_init();
	tty_init();
	print_cpuid();
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
	ps2_init();
	ide_init();
	//if (has_apic)
	//	start_ap();
	*(uint32_t*)(0xFFFFF000) = 0; /* remove identity paging at 0x00000000 */
	sched_init();
	{
		char name[64];
		uint32_t cpuid = curcpu();
		snprintf(name, sizeof(name), "idle%" PRIu32, cpuid);
		const char *argv[] = {"idle", NULL};
		const char *envp[] = {NULL};
		idlethread = kproc_create(name, idle_loop, argv, envp);
		idlethread->pri = 255;
		CPUMASK_CLEAR(&idlethread->affinity);
		CPUMASK_SET(&idlethread->affinity, cpuid, 1);
		sched_add(idlethread);
	}
	idlethread->state = THREAD_RUNNING;
	struct thread *thread;
	{
		struct file *file;
		struct fs_node *elf;
		assert(!vfs_getnode(NULL, "/bin/sh", &elf), "can't open /bin/sh");
		file = malloc(sizeof(*file), 0);
		file->op = elf->fop;
		file->node = elf;
		file->off = 0;
		file->refcount = 1;
		const char *argv[] = {"/bin/sh", NULL};
		const char *envp[] = {NULL};
		thread = uproc_create_elf("sh", file, argv, envp);
		sched_add(thread);
		file_decref(file);
	}
	/* added after init to avoid buggy scheduling on page fault interrupt */
	sched_run(idlethread);
	sched_run(thread);
	idle_loop();
	panic("post idle loop\n");

#if 0
	uint8_t buf[512];
	ide_read_sectors(0, 1, 0, (uint8_t*)buf);
	for (size_t i = 0; i < 512; ++i)
		printf("%02x, ", buf[i]);
	printf("\n");
#endif

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

}

void x86_panic(uint32_t *esp, const char *file, const char *line, const char *fn, const char *fmt, ...)
{
	va_list va_arg;

	va_start(va_arg, fmt);
	vprintf(fmt, va_arg);
	printf("%s@%s:%s\n", fn, file, line);
	printf("EAX: %08" PRIx32 " EBX: %08" PRIx32 " ECX: %08" PRIx32 " EDX: %08" PRIx32 "\n", esp[8], esp[5], esp[7], esp[6]);
	printf("ESI: %08" PRIx32 " EDI: %08" PRIx32 " ESP: %08" PRIx32 " EBP: %08" PRIx32 "\n", esp[2], esp[1], esp[4], esp[3]);
	printf("EIP: %08" PRIx32 "\n", esp[0]);

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
	thread->tf.eax = 0;
	thread->tf.ebx = 0;
	thread->tf.ecx = 0;
	thread->tf.edx = 0;
	thread->tf.esi = 0;
	thread->tf.edi = 0;
	thread->tf.esp = (uint32_t)&thread->stack[thread->stack_size];
	thread->tf.ebp = (uint32_t)&thread->stack[thread->stack_size];
	thread->tf.eip = (uint32_t)thread->proc->entrypoint;
}

void init_trapframe_kern(struct thread *thread)
{
	init_trapframe(thread);
	thread->tf.cs = 0x08;
	thread->tf.ds = 0x10;
	thread->tf.es = 0x10;
	thread->tf.fs = 0x10;
	thread->tf.gs = 0x10;
	thread->tf.ss = 0x10;
	thread->tf.ef = getef() | (1 << 9); /* IF */
}

void init_trapframe_user(struct thread *thread)
{
	init_trapframe(thread);
	thread->tf.cs = 0x1B;
	thread->tf.ds = 0x23;
	thread->tf.es = 0x23;
	thread->tf.fs = 0x23;
	thread->tf.gs = 0x23;
	thread->tf.ss = 0x23;
	thread->tf.ef = getef() | (1 << 9) | (3 << 12); /* IF, CPL=3 */
}

uint32_t curcpu(void)
{
	uint32_t eax, ebx, ecx, edx;
	__cpuid(1, eax, ebx, ecx, edx);
	return ebx >> 24;
}
