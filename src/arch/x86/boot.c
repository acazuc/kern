#include "gdt.h"
#include "idt.h"
#include "dev/pic/pic.h"
#include "dev/pit/pit.h"
#include "dev/vga/vga.h"
#include "dev/com/com.h"
#include "dev/ps2/ps2.h"
#include "sys/std.h"
#include "io.h"
#include "multiboot.h"

static void print_multiboot(struct multiboot_info *mb_info)
{
	printf("mb_info: %p\n", mb_info);
	printf("multiboot flags: %08x\n", mb_info->flags);
	if (mb_info->flags & MULTIBOOT_INFO_CMDLINE)
		printf("cmdline: %s\n", (char*)mb_info->cmdline);
	if (mb_info->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME)
		printf("bootloader: %s\n", (char*)mb_info->boot_loader_name);
	printf("BIOS mem: %08x - %08x\n", mb_info->mem_lower, mb_info->mem_upper);
	uint64_t total_mem = 0;
	if (mb_info->flags & MULTIBOOT_INFO_MEM_MAP)
	{
		for (size_t i = 0; i < mb_info->mmap_length; i += sizeof(multiboot_memory_map_t))
		{
			multiboot_memory_map_t *mmap = (multiboot_memory_map_t*)(mb_info->mmap_addr + i + 4);
			printf("mmap: %016llx @ %016llx (%08x)\n", mmap->len, mmap->addr, mmap->type);
			if (mmap->type == 0)
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

void boot(struct multiboot_info *mb_info)
{
	vga_init();
	printf("x86 boot\n");
	print_multiboot(mb_info);
	pic_init(0x20, 0x28);
	gdt_init();
	idt_init();
	pit_init();
	com_init();
	ps2_init();
	printf("boot sequence completed; enabling interrupts\n");
	__asm__ volatile ("sti");
}
