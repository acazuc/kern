#include <elf.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/file.h>
#include <stdio.h>

#include "arch/arch.h"

struct thread *elf_createproc(struct file *file)
{
	uint8_t *data = NULL;
	size_t len = 0;
	int r;
	do
	{
		uint8_t *tmp = realloc(data, len + 4096, M_ZERO);
		assert(tmp, "can't allocate elf file data\n");
		data = tmp;
		r = file->op->read(file, data + len, 4096);
		assert(r >= 0, "failed to read from elf file\n");
		len += r;
	} while (r == 4096);
	printf("got file of length %lu\n", len);
	assert(data, "no data has been read\n");
	assert(len >= sizeof(Elf32_Ehdr), "file too short (no header)\n");
	Elf32_Ehdr *hdr = (Elf32_Ehdr*)data;
	assert(hdr->e_ident[EI_MAG0] == ELFMAG0
	    && hdr->e_ident[EI_MAG1] == ELFMAG1
	    && hdr->e_ident[EI_MAG2] == ELFMAG2
	    && hdr->e_ident[EI_MAG3] == ELFMAG3,
	    "invalid header magic\n");
	assert(hdr->e_ident[EI_CLASS] == ELFCLASS32, "invalid class\n");
	assert(hdr->e_ident[EI_DATA] == ELFDATA2LSB, "invalid data mode\n");
	assert(hdr->e_ident[EI_VERSION] == EV_CURRENT, "invalid ident version\n");
	assert(hdr->e_type == ET_DYN, "not dynamic\n");
	assert(hdr->e_machine == EM_386, "not i386\n");
	assert(hdr->e_version == EV_CURRENT, "invalid version\n");

	assert(hdr->e_shoff < len, "section header outside of file\n");
	assert(hdr->e_shentsize >= sizeof(Elf32_Shdr), "section entry size too low\n");
	assert(hdr->e_shoff + hdr->e_shentsize * hdr->e_shnum <= len, "section header end outside of file\n");

	assert(hdr->e_phoff < len, "program header outside of file\n");
	assert(hdr->e_phentsize >= sizeof(Elf32_Phdr), "program entry size too low\n");
	assert(hdr->e_phoff + hdr->e_phentsize * hdr->e_phnum <= len, "program header end outside of file\n");

	struct vmm_ctx *vmm_ctx = create_vmm_ctx();
	assert(vmm_ctx, "can't create vmm ctx\n");

	size_t base_addr = 0x100000; /* XXX: ask to vmm_ctx for real base addr */
	for (size_t i = 0; i < hdr->e_phnum; ++i)
	{
		Elf32_Phdr *phdr = (Elf32_Phdr*)(&data[hdr->e_phoff + hdr->e_phentsize * i]);
		if (phdr->p_type == PT_LOAD)
		{
			size_t addr = phdr->p_vaddr;
			addr -= addr % PAGE_SIZE;
			size_t size = phdr->p_memsz;
			size += phdr->p_vaddr - addr;
			size += PAGE_SIZE - 1;
			size -= size % PAGE_SIZE;
			printf("allocate %lx bytes at %lx\n", size, addr);
			void *ptr = vmalloc_user(vmm_ctx, (void*)(addr + base_addr), size);
			assert(ptr, "can't allocate program header\n");
			void *dst = vmap_user(vmm_ctx, ptr, size);
			assert(dst, "can't vmap user\n");
			/* XXX memcpy */
			vunmap(dst, size);
		}
	}

	//while (1) {}
	return NULL;
}
