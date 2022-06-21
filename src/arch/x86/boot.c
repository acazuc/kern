#include "x86.h"
#include "dev/pic/pic.h"
#include "dev/pit/pit.h"
#include "dev/vga/vga.h"
#include "dev/com/com.h"
#include "dev/ps2/ps2.h"
#include "dev/ide/ide.h"
#include "sys/std.h"
#include "io.h"
#include "multiboot.h"

#include <stdbool.h>
#include <cpuid.h>

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
		bool first = true;
		for (size_t i = 0; i < 32; ++i)
		{
			if (!(edx & (1 << i)))
				continue;
			if (first)
				first = false;
			else
				printf(", ");
			printf("%s", cpuid_feature_edx_str[i]);
		}
		for (size_t i = 0; i < 32; ++i)
		{
			if (!(ecx & (1 << i)))
				continue;
			if (first)
				first = false;
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

void boot(struct multiboot_info *mb_info)
{
	vga_init();
	printf("\n");
	printf("x86 boot @ %p\n", boot);
	print_multiboot(mb_info);
	print_cpuid();
	pic_init(0x20, 0x28);
	gdt_init();
	idt_init();
	pit_init();
	com_init();
	ps2_init();
	ide_init();
	uint32_t mem_size = mb_get_memory_map_size(mb_info);
	if (!mem_size)
		panic("can't get memory map\n");
	if (mem_size < 0x1000000)
		panic("can't get 16MB of memory\n");
	paging_init(0x1000000, mem_size - 0x1000000);
	__asm__ volatile ("sti");



	/*uint8_t buf[512];
	ide_read_sectors(0, 1, 0, (uint8_t*)buf);
	for (size_t i = 0; i < 512; ++i)
		printf("%02x, ", buf[i]);
	printf("\n");*/
}

void x86_panic(uint32_t *esp, const char *file, const char *line, const char *fn, const char *fmt, ...)
{
	va_list va_arg;

	va_start(va_arg, fmt);
	vprintf(fmt, va_arg);
	printf("%s@%s:%s\n", fn, file, line);
	printf("EAX: %08lx EBX: %08lx ECX: %08lx EDX: %08lx\n", esp[9], esp[6], esp[8], esp[7]);
	printf("ESI: %08lx EDI: %08lx ESP: %08lx EBP: %08lx\n", esp[3], esp[2], esp[5], esp[4]);
	printf("EIP: %08lx\n", esp[0]);

#if 0
	void *ptr[64];
	bool end = false;

#define N1(n) if (!end) { ptr[n] = __builtin_return_address(n); if (!ptr[n]) { end = true; } }
#define N2(n) N1(n + 0) N1(n + 1)
#define N4(n) N2(n + 0) N2(n + 2)
#define N8(n) N4(n + 0) N4(n + 4)
#define N16(n) N8(n + 0) N8(n + 8)
#define N32(n) N16(n + 0) N16(n + 16)
#define N64(n) N32(n + 0) N32(n + 32)

	N64(0)

	for (size_t i = 0; i < 32; ++i)
	{
		if (ptr[i])
			printf("[%u] %p\n", i, ptr[i]);
	}
#endif

infl:
	__asm__ volatile ("cli;hlt");
	goto infl;
}
