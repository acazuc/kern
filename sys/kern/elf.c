#include <sys/file.h>
#include <inttypes.h>
#include <sys/std.h>
#include <sys/vmm.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <arch.h>
#include <elf.h>
#include <vfs.h>

struct elf_dynamic_ctx
{
	struct vmm_ctx *vmm_ctx;
	const uint8_t *data;
	size_t base_addr;
	const Elf32_Dyn *symtab;
	const Elf32_Dyn *syment;
	const Elf32_Dyn *strtab;
	const Elf32_Dyn *strsz;
};

static const Elf32_Dyn *get_dyn(const Elf32_Phdr *phdr, const uint8_t *data, Elf32_Sword tag)
{
	for (size_t i = 0; i < phdr->p_filesz; i += sizeof(Elf32_Dyn))
	{
		const Elf32_Dyn *dyn = (const Elf32_Dyn*)&data[phdr->p_offset + i];
		if (dyn->d_tag == tag)
			return dyn;
	}
	return NULL;
}

static void handle_dt_rel(struct elf_dynamic_ctx *ctx, const Elf32_Dyn *rel, const Elf32_Dyn *relsz, size_t size)
{
	for (size_t i = 0; i < relsz->d_un.d_val; i += size)
	{
		const Elf32_Rel *r = (const Elf32_Rel*)&ctx->data[rel->d_un.d_ptr + i]; /* XXX test overflow */
		uint32_t reladdr = ctx->base_addr + r->r_offset;
		uint32_t map_addr = reladdr - reladdr % PAGE_SIZE;
		size_t map_size = 8 + reladdr;
		if (map_size % PAGE_SIZE < reladdr % PAGE_SIZE)
			map_size = PAGE_SIZE * 2;
		else
			map_size = PAGE_SIZE;
		switch (ELF32_R_TYPE(r->r_info))
		{
			case R_386_RELATIVE:
			{
				void *ptr = vmap_user(ctx->vmm_ctx, (void*)map_addr, map_size);
				assert(ptr, "can't vmap reloc\n");
				*(uint32_t*)((uint8_t*)ptr + (reladdr - map_addr)) += ctx->base_addr;
				vunmap(ptr, map_size);
				break;
			}
			case R_386_JMP_SLOT:
			{
				assert(ctx->symtab, "no symtab on jmpslot rel\n");
				assert(ctx->strtab, "no strtab on jmpslot rel\n");
				uint32_t symidx = ELF32_R_SYM(r->r_info);
				const Elf32_Sym *sym = (const Elf32_Sym*)&ctx->data[ctx->symtab->d_un.d_ptr + symidx * ctx->syment->d_un.d_val]; /* XXX test overflow */
				void *ptr = vmap_user(ctx->vmm_ctx, (void*)map_addr, map_size);
				assert(ptr, "can't vmap reloc\n");
				*(uint32_t*)((uint8_t*)ptr + (reladdr - map_addr)) = ctx->base_addr + sym->st_value;
				vunmap(ptr, map_size);
				break;
			}
			case R_386_GLOB_DAT:
			{
				assert(ctx->symtab, "no symtab on globdat rel\n");
				assert(ctx->strtab, "no strtab on globdat rel\n");
				uint32_t symidx = ELF32_R_SYM(r->r_info);
				const Elf32_Sym *sym = (const Elf32_Sym*)&ctx->data[ctx->symtab->d_un.d_ptr + symidx * ctx->syment->d_un.d_val]; /* XXX test overflow */
				void *ptr = vmap_user(ctx->vmm_ctx, (void*)map_addr, map_size);
				assert(ptr, "can't vmap reloc\n");
				*(uint32_t*)((uint8_t*)ptr + (reladdr - map_addr)) = ctx->base_addr + sym->st_value;
				vunmap(ptr, map_size);
				break;
			}
			default:
				panic("unhandled reloc type: %" PRIx32 "\n", ELF32_R_TYPE(r->r_info));
				break;
		}
	}
}

static void handle_dynamic(struct vmm_ctx *vmm_ctx, const uint8_t *data, size_t base_addr, const Elf32_Phdr *phdr, int is_interp)
{
	/* XXX: one DT_HASH */
	/* XXX: one DT_RELA */
	/* XXX: one DT_RELASZ if DT_RELA */
	/* XXX: one DT_RELAENT if DT_RELA */
	struct elf_dynamic_ctx ctx;
	ctx.vmm_ctx = vmm_ctx;
	ctx.data = data;
	ctx.base_addr = base_addr;
	ctx.symtab = NULL;
	ctx.syment = NULL;
	ctx.strtab = NULL;
	ctx.strsz = NULL;
	for (size_t i = 0; i < phdr->p_filesz; i += sizeof(Elf32_Dyn))
	{
		const Elf32_Dyn *dyn = (const Elf32_Dyn*)&data[phdr->p_offset + i];
		switch (dyn->d_tag)
		{
			case DT_NULL:
				goto enditer;
			case DT_STRTAB:
				assert(!ctx.strtab, "multiple strtab\n");
				ctx.strtab = dyn;
				break;
			case DT_STRSZ:
				assert(!ctx.strsz, "multiple strsz\n");
				ctx.strsz = dyn;
				break;
			case DT_SYMTAB:
				assert(!ctx.symtab, "multiple symtab\n");
				ctx.symtab = dyn;
				break;
			case DT_SYMENT:
				assert(!ctx.syment, "multiple syment\n");
				ctx.syment = dyn;
				break;
			case DT_REL:
			case DT_RELSZ:
			case DT_RELENT:
			case DT_JMPREL:
			case DT_PLTREL:
			case DT_PLTRELSZ:
				break;
			case DT_NEEDED:
				assert(!is_interp, "DT_NEEDED in interprer\n");
				break;
			case DT_GNU_HASH:
			case DT_DEBUG:
			case DT_TEXTREL:
			case DT_FLAGS:
			case DT_FLAGS_1:
			case DT_RELCOUNT:
			case DT_PLTGOT:
				/* XXX */
				break;
			default:
				panic("unhandled dyn tag %" PRIx32 "\n", dyn->d_tag);
				break;
		}
	}
enditer:
	assert(!ctx.strtab || ctx.strsz, "strtab and no strsz\n");
	assert(!ctx.symtab || ctx.syment, "symtab and not syment\n");
	const Elf32_Dyn *rel = get_dyn(phdr, data, DT_REL);
	if (rel)
	{
		const Elf32_Dyn *relsz = get_dyn(phdr, data, DT_RELSZ);
		const Elf32_Dyn *relent = get_dyn(phdr, data, DT_RELENT);
		assert(relsz, "no relsz\n");
		assert(relent, "no relent\n");
		handle_dt_rel(&ctx, rel, relsz, relent->d_un.d_val);
	}
	const Elf32_Dyn *jmprel = get_dyn(phdr, data, DT_JMPREL);
	if (jmprel)
	{
		const Elf32_Dyn *pltrel = get_dyn(phdr, data, DT_PLTREL);
		const Elf32_Dyn *pltrelsz = get_dyn(phdr, data, DT_PLTRELSZ);
		assert(pltrel, "no pltrel\n");
		assert(pltrelsz, "no pltrelsz\n");
		switch (pltrel->d_un.d_val)
		{
			case DT_REL:
				handle_dt_rel(&ctx, jmprel, pltrelsz, sizeof(Elf32_Rel));
				break;
			case DT_RELA:
				panic("unsupported DT_RELA\n");
				break;
			default:
				panic("invalid pltrel type: %" PRId32 "\n", pltrel->d_un.d_val);
		}
	}
}

static int createctx(struct file *file, struct vmm_ctx *vmm_ctx, void **entry, int is_interp)
{
	uint8_t *data = NULL;
	size_t len = 0;
	int r;
	do
	{
		uint8_t *tmp = realloc(data, len + 4096, 0);
		assert(tmp, "can't allocate elf file data\n");
		data = tmp;
		r = file->op->read(file, data + len, 4096);
		assert(r >= 0, "failed to read from elf file\n");
		len += r;
	} while (r > 0);
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

	const Elf32_Phdr *interp = NULL;
	for (size_t i = 0; i < hdr->e_phnum; ++i)
	{
		const Elf32_Phdr *phdr = (const Elf32_Phdr*)(&data[hdr->e_phoff + hdr->e_phentsize * i]);
		if (phdr->p_type != PT_INTERP)
			continue;
		assert(!interp, "more than one interp\n");
		interp = phdr;
	}

	if (interp)
	{
		assert(!is_interp, "recursive interpreter not supported\n");
		const char *path = (const char*)&data[interp->p_offset]; /* XXX check bounds */
		if (memchr(path, '\0', interp->p_filesz) == NULL)
		{
			free(data);
			return EINVAL;
		}
		struct fs_node *node;
		assert(!vfs_getnode(NULL, path, &node), "can't find interp node\n");
		struct file *interp_f = malloc(sizeof(*interp_f), M_ZERO);
		interp_f->node = node;
		interp_f->op = node->fop;
		interp_f->refcount = 1;
		int ret = createctx(interp_f, vmm_ctx, entry, 1);
		file_decref(interp_f);
		free(data);
		return ret;
	}

	size_t base_addr = 0x100000; /* XXX ASLR */
	for (size_t i = 0; i < hdr->e_phnum; ++i)
	{
		Elf32_Phdr *phdr = (Elf32_Phdr*)(&data[hdr->e_phoff + hdr->e_phentsize * i]);
		switch (phdr->p_type)
		{
			case PT_LOAD:
			{
				/* XXX: should be using file->mmap(file, vmm_ctx, base_addr) */
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
			case PT_DYNAMIC:
			{
				handle_dynamic(vmm_ctx, data, base_addr, phdr, is_interp);
				break;
			}
		}
	}

	*entry = (void*)base_addr + hdr->e_entry;
	free(data);
	return 0;
}

int elf_createctx(struct file *file, struct vmm_ctx *vmm_ctx, void **entry)
{
	return createctx(file, vmm_ctx, entry, 0);
}
