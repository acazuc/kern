#include <sys/file.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <elf.h>

#include "arch/arch.h"

int elf_createctx(struct file *file, struct vmm_ctx *vmm_ctx, void **entry)
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

	size_t base_addr = 0x100000; /* XXX: ask to vmm_ctx for real base addr */
	Elf32_Addr rel = 0;
	Elf32_Word relsz = 0;
	Elf32_Word relent = 0;
	for (size_t i = 0; i < hdr->e_phnum; ++i)
	{
		Elf32_Phdr *phdr = (Elf32_Phdr*)(&data[hdr->e_phoff + hdr->e_phentsize * i]);
		switch (phdr->p_type)
		{
			case PT_LOAD:
			{
				size_t addr = phdr->p_vaddr;
				addr -= addr % PAGE_SIZE;
				size_t size = phdr->p_memsz;
				size += phdr->p_vaddr - addr;
				size += PAGE_SIZE - 1;
				size -= size % PAGE_SIZE;
				void *ptr = vmalloc_user(vmm_ctx, (void*)(addr + base_addr), size);
				assert(ptr, "can't allocate program header\n");
				/*
				 * XXX: should be done as a user_cpy in vmm, allowing
				 * page per page cpy in case of large elf file
				 */
				void *dst = vmap_user(vmm_ctx, ptr, size);
				assert(dst, "can't vmap user\n");
				memset(dst, 0, size); /* safety first */
				memcpy(dst + phdr->p_vaddr - addr, &data[phdr->p_offset], phdr->p_filesz);
				vunmap(dst, size);
				break;
			}
			case PT_GNU_RELRO:
			{
				for (size_t i = 0; i < phdr->p_filesz; i += sizeof(Elf32_Dyn))
				{
					Elf32_Dyn *dyn = (Elf32_Dyn*)&data[phdr->p_offset + i];
					switch (dyn->d_tag)
					{
						case DT_NULL:
							goto nextph;
						case DT_NEEDED:
							panic("unhandled DT_NEEDED\n");
							break;
						case DT_REL:
							rel = dyn->d_un.d_ptr;
							break;
						case DT_RELSZ:
							relsz = dyn->d_un.d_val;
							break;
						case DT_RELENT:
							relent = dyn->d_un.d_val;
							break;
						case DT_GNU_HASH:
						case DT_STRTAB:
						case DT_SYMTAB:
						case DT_STRSZ:
						case DT_SYMENT:
						case DT_DEBUG:
						case DT_TEXTREL:
						case DT_FLAGS:
						case DT_FLAGS_1:
						case DT_RELCOUNT:
							/* XXX */
							break;
						default:
							panic("unhandled dyn tag %lx\n", dyn->d_tag);
							break;
					}
				}
				break;
			}
		}
nextph:;
	}

	if (relsz)
	{
		assert(relent == sizeof(Elf32_Rel), "invalid relent value\n");
		for (size_t i = 0; i < relsz; i += relent)
		{
			Elf32_Rel *r = (Elf32_Rel*)&data[rel + i];
			uint32_t reladdr = base_addr + r->r_offset;
			uint32_t addr = reladdr - reladdr % PAGE_SIZE;
			size_t size = 8 + reladdr;
			if (size % PAGE_SIZE < reladdr % PAGE_SIZE)
				size = PAGE_SIZE * 2;
			else
				size = PAGE_SIZE;
			switch (ELF32_R_TYPE(r->r_info))
			{
				case R_386_RELATIVE:
				{
					void *ptr = vmap_user(vmm_ctx, (void*)addr, size);
					assert(ptr, "can't vmap reloc\n");
					*(uint32_t*)((uint8_t*)ptr + (reladdr - addr)) += base_addr;
					vunmap(ptr, size);
					break;
				}
				default:
					panic("unhandled reloc type: %lx\n", ELF32_R_TYPE(r->r_info));
					break;
			}
		}
	}

	*entry = (void*)base_addr + hdr->e_entry;
	free(data);
	return 0;
}
