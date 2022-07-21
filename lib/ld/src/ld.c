#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <elf.h>

#define PAGE_SIZE 4096 /* XXX in header */

#define O_RDONLY (1 << 0)
#define O_WRONLY (1 << 1)
#define O_RDWR   (O_RDONLY | O_WRONLY)
#define O_APPEND (1 << 2)
#define O_ASYNC  (1 << 3)
#define O_CREAT  (1 << 4)
#define O_TRUNC  (1 << 5)

extern int32_t errno;
int g_stdout;

/* containing a dyn object (either target binary or dependency) */
struct elf
{
	char name[256];
	int fd;
	size_t addr;
	Elf32_Ehdr ehdr;
	Elf32_Phdr phdr[16]; /* dynamic ? */
	struct elf *dep[256]; /* dynamic ? */
	size_t dep_nb;
	const Elf32_Dyn *symtab;
	const Elf32_Dyn *syment;
	const Elf32_Dyn *strtab;
	const Elf32_Dyn *strsz;
};

struct elf g_elf[256]; /* maximum number of ELF objects loaded */
size_t g_elf_nb;

int exit(int error_code);
int read(int fd, void *buffer, size_t count);
int write(int fd, const void *buffer, size_t count);
int open(const char *path, int flags, ...);
int close(int fd);
int lseek(int fd, off_t off, int whence);
void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);

static int elf_read(struct elf *elf);
static int elf_resolve(struct elf *elf);
static int elf_map(struct elf *elf);

static size_t strlen(const char *s)
{
	size_t i = 0;
	while (s[i])
		i++;
	return i;
}

static void *memset(void *d, int c, size_t len)
{
	for (size_t i = 0; i < len; ++i)
		((uint8_t*)d)[i] = c;
	return d;
}

static size_t strlcpy(char *d, const char *s, size_t n)
{
	size_t i;
	for (i = 0; s[i] && i < n - 1; ++i)
		d[i] = s[i];
	d[i] = '\0';
	while (s[i])
		++i;
	return i;
}

static void puts(const char *s)
{
	write(g_stdout, s, strlen(s));
}

static struct elf *getelf(void)
{
	if (g_elf_nb >= sizeof(g_elf) / sizeof(*g_elf))
	{
		puts("no more elf available\n");
		return NULL;
	}
	struct elf *elf = &g_elf[g_elf_nb++];
	memset(elf, 0, sizeof(*elf));
	return elf;
}

static uint32_t find_dep_sym(struct elf *elf, const char *name)
{
	for (size_t i = 0; i < elf->dep_nb; ++i)
	{
		struct elf *dep = elf->dep[i];
		if (dep->symtab && dep->strtab)
		{
			//
		}
		uint32_t dep_sym = find_dep_sym(dep, name);
		if (dep_sym)
			return dep_sym;
	}
	return 0;
}

static const Elf32_Dyn *get_dyn(struct elf *elf, const Elf32_Phdr *phdr, Elf32_Sword tag)
{
	for (size_t i = 0; i < phdr->p_filesz; i += sizeof(Elf32_Dyn))
	{
		const Elf32_Dyn *dyn = (const Elf32_Dyn*)(elf->addr + phdr->p_offset + i);
		if (dyn->d_tag == tag)
			return dyn;
	}
	return NULL;
}

static int handle_dt_rel(struct elf *elf, const Elf32_Dyn *rel, const Elf32_Dyn *relsz, size_t size)
{
	for (size_t i = 0; i < relsz->d_un.d_val; i += size)
	{
		const Elf32_Rel *r = (const Elf32_Rel*)(elf->addr + rel->d_un.d_ptr + i);
		uint32_t *addr = (uint32_t*)(elf->addr + r->r_offset);
		switch (ELF32_R_TYPE(r->r_info))
		{
			case R_386_RELATIVE:
				*(uint32_t*)addr += elf->addr;
				break;
			case R_386_JMP_SLOT:
			{
				if (!elf->symtab)
				{
					puts("no symtab on jmpslot rel\n");
					return 1;
				}
				if (!elf->strtab)
				{
					puts("no strtab on jmpslot rel\n");
					return 1;
				}
				uint32_t symidx = ELF32_R_SYM(r->r_info);
				const Elf32_Sym *sym = (const Elf32_Sym*)(elf->addr + elf->symtab->d_un.d_ptr + symidx * elf->syment->d_un.d_val);
				if (sym->st_shndx == SHN_UNDEF)
				{
					const char *name = (const char*)(elf->addr + elf->strtab->d_un.d_ptr + sym->st_name);
					uint32_t value = find_dep_sym(elf, name);
					if (!value)
					{
						puts("symbol not found: ");
						puts(name);
						puts("\n");
						return 1;
					}
					*addr = value;
				}
				else
				{
					*addr = elf->addr + sym->st_value;
				}
				break;
			}
			case R_386_GLOB_DAT:
			{
				if (!elf->symtab)
				{
					puts("no symtab on globdat rel\n");
					return 1;
				}
				if (!elf->strtab)
				{
					puts("no strtab on globdat rel\n");
					return 1;
				}
				uint32_t symidx = ELF32_R_SYM(r->r_info);
				const Elf32_Sym *sym = (const Elf32_Sym*)(elf->addr + elf->symtab->d_un.d_ptr + symidx * elf->syment->d_un.d_val);
				*addr = elf->addr + sym->st_value;
				break;
			}
			default:
				puts("unhalded reloc type\n");
				return 1;
		}
	}
	return 0;
}

static int handle_pt_dynamic(struct elf *elf, const Elf32_Phdr *phdr)
{
	/* XXX: one DT_HASH */
	/* XXX: one DT_RELA */
	/* XXX: one DT_RELASZ if DT_RELA */
	/* XXX: one DT_RELAENT if DT_RELA */
	for (size_t i = 0; i < phdr->p_filesz; i += sizeof(Elf32_Dyn))
	{
		const Elf32_Dyn *dyn = (const Elf32_Dyn*)(elf->addr + phdr->p_offset + i);
		switch (dyn->d_tag)
		{
			case DT_NULL:
				goto enditer;
			case DT_STRTAB:
				if (elf->strtab)
				{
					puts("multiple strtab\n");
					return 1;
				}
				elf->strtab = dyn;
				break;
			case DT_STRSZ:
				if (elf->strsz)
				{
					puts("multiple strsz\n");
					return 1;
				}
				elf->strsz = dyn;
				break;
			case DT_SYMTAB:
				if (elf->symtab)
				{
					puts("multiple symtab\n");
					return 1;
				}
				elf->symtab = dyn;
				break;
			case DT_SYMENT:
				if (elf->syment)
				{
					puts("multiple syment\n");
					return 1;
				}
				elf->syment = dyn;
				break;
			case DT_REL:
			case DT_RELSZ:
			case DT_RELENT:
			case DT_JMPREL:
			case DT_PLTREL:
			case DT_PLTRELSZ:
			case DT_NEEDED:
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
				puts("unhandled dyn tag\n");
				return 1;
		}
	}
enditer:
	if (elf->strtab && !elf->strsz)
	{
		puts("strtab and no strsz\n");
		return 1;
	}
	if (elf->symtab && !elf->syment)
	{
		puts("symtab and no syment\n");
		return 1;
	}
	for (size_t i = 0; i < phdr->p_filesz; i += sizeof(Elf32_Dyn))
	{
		const Elf32_Dyn *dyn = (const Elf32_Dyn*)(elf->addr + phdr->p_offset + i);
		if (dyn->d_tag != DT_NEEDED)
			continue;
		if (!elf->strtab)
		{
			puts("DT_NEEDED and no strtab\n");
			return 1;
		}
		if (elf->dep_nb >= sizeof(elf->dep) / sizeof(*elf->dep))
		{
			puts("no more dep available\n");
			return 1;
		}
		const char *name = (const char*)(elf->addr + elf->strtab->d_un.d_ptr + dyn->d_un.d_val);
		char filepath[256] = "/lib/"; /* XXX MAXPATHLEN */
		if (strlcpy(filepath + 5, name, sizeof(filepath) - 5) >= sizeof(filepath) - 5)
		{
			puts("filepath too long\n");
			return 1;
		}
		int fd = open(filepath, O_RDONLY);
		if (fd < 0)
		{
			puts("failed to open dependency\n");
			return 1;
		}
		struct elf *dep = getelf();
		if (!dep)
		{
			close(fd);
			puts("failed to get dep\n");
			return 1;
		}
		dep->fd = fd;
		puts("handling dep: ");
		puts(name);
		puts("\n");
		if (elf_read(dep))
		{
			puts("failed to read dep\n");
			return 1;
		}
		if (elf_map(dep))
		{
			puts("failed to map dep\n");
			return 1;
		}
		if (elf_resolve(dep))
		{
			puts("failed to resolve dep\n");
			return 1;
		}
		elf->dep[elf->dep_nb++] = dep;
		puts("handled\n");
	}
	const Elf32_Dyn *rel = get_dyn(elf, phdr, DT_REL);
	if (rel)
	{
		const Elf32_Dyn *relsz = get_dyn(elf, phdr, DT_RELSZ);
		const Elf32_Dyn *relent = get_dyn(elf, phdr, DT_RELENT);
		if (!relsz)
		{
			puts("no relsz on rel\n");
			return 1;
		}
		if (!relent)
		{
			puts("no relent on rel\n");
			return 1;
		}
		if (handle_dt_rel(elf, rel, relsz, relent->d_un.d_val))
		{
			puts("failed to handle DT_REL\n");
			return 1;
		}
	}
	const Elf32_Dyn *jmprel = get_dyn(elf, phdr, DT_JMPREL);
	if (jmprel)
	{
		const Elf32_Dyn *pltrel = get_dyn(elf, phdr, DT_PLTREL);
		const Elf32_Dyn *pltrelsz = get_dyn(elf, phdr, DT_PLTRELSZ);
		if (!pltrel)
		{
			puts("no pltrel on jmprel\n");
			return 1;
		}
		if (!pltrelsz)
		{
			puts("no pltrelsz on jmprel\n");
			return 1;
		}
		switch (pltrel->d_un.d_val)
		{
			case DT_REL:
				if (handle_dt_rel(elf, jmprel, pltrelsz, sizeof(Elf32_Rel)))
				{
					puts("failed to handle PLT_REL DT_REL\n");
					return 1;
				}
				break;
			case DT_RELA:
				puts("unsupported DT_RELA\n");
				return 1;
			default:
				puts("unhandled pltrel type\n");
				return 1;
		}
	}
	return 0;
}

static int handle_pt_load(struct elf *elf, const Elf32_Phdr *phdr)
{
	if (phdr->p_align % PAGE_SIZE != 0)
	{
		puts("PT_LOAD invalid align\n");
		return 1;
	}
	size_t addr = phdr->p_vaddr;
	addr -= addr % PAGE_SIZE;
	size_t size = phdr->p_memsz;
	size += phdr->p_vaddr - addr;
	size += PAGE_SIZE - 1;
	size -= size % PAGE_SIZE;
	void *dst = (void*)(elf->addr + addr);
	void *ptr = mmap(dst, size, 0/* XXX */, 0 /* XXX */, elf->fd, phdr->p_offset);
	if (ptr == (void*)-1)
	{
		puts("mmap failed\n");
		return 1;
	}
	if (ptr != dst)
	{
		puts("can't mmap at wanted location\n");
		return 1;
	}
	return 0;
}

static int elf_read(struct elf *elf)
{
	off_t off = lseek(elf->fd, 0, SEEK_END);
	if (off < 0)
	{
		puts("can't get file size\n");
		return 1;
	}
	size_t len = (size_t)off;
	if (lseek(elf->fd, 0, SEEK_SET) < 0)
	{
		puts("can't reset file to beggining\n");
		return 1;
	}
	if (read(elf->fd, &elf->ehdr, sizeof(elf->ehdr)) != sizeof(elf->ehdr))
	{
		puts("failed to read ehdr\n");
		return 1;
	}
	if (elf->ehdr.e_ident[EI_MAG0] != ELFMAG0
	 || elf->ehdr.e_ident[EI_MAG1] != ELFMAG1
	 || elf->ehdr.e_ident[EI_MAG2] != ELFMAG2
	 || elf->ehdr.e_ident[EI_MAG3] != ELFMAG3)
	{
		puts("invalid header magic\n");
		return 1;
	}
	if (elf->ehdr.e_ident[EI_CLASS] != ELFCLASS32)
	{
		puts("invalid class\n");
		return 1;
	}
	if (elf->ehdr.e_ident[EI_DATA] != ELFDATA2LSB)
	{
		puts("invalid data mode\n");
		return 1;
	}
	if (elf->ehdr.e_ident[EI_VERSION] != EV_CURRENT)
	{
		puts("invalid ident version\n");
		return 1;
	}
	if (elf->ehdr.e_type != ET_DYN)
	{
		puts("not dynamic\n");
		return 1;
	}
	if (elf->ehdr.e_machine != EM_386)
	{
		puts("not i386\n");
		return 1;
	}
	if (elf->ehdr.e_version != EV_CURRENT)
	{
		puts("invalid version\n");
		return 1;
	}
	if (elf->ehdr.e_shoff >= len)
	{
		puts("section header outside of file\n");
		return 1;
	}
	if (elf->ehdr.e_shentsize < sizeof(Elf32_Shdr))
	{
		puts("section entry size too low\n");
		return 1;
	}
	if (elf->ehdr.e_shoff + elf->ehdr.e_shentsize * elf->ehdr.e_shnum > len)
	{
		puts("section header end outside of file\n");
		return 1;
	}
	if (elf->ehdr.e_phoff >= len)
	{
		puts("program header outside of file\n");
		return 1;
	}
	if (elf->ehdr.e_phentsize < sizeof(Elf32_Phdr))
	{
		puts("program entry size too low\n");
		return 1;
	}
	if (elf->ehdr.e_phoff + elf->ehdr.e_phentsize * elf->ehdr.e_phnum > len)
	{
		puts("program header end outside of file\n");
		return 1;
	}
	if (elf->ehdr.e_phnum > sizeof(elf->phdr) / sizeof(*elf->phdr))
	{
		puts("too much phdr\n");
		return 1;
	}
	for (size_t i = 0; i < elf->ehdr.e_phnum; ++i)
	{
		ssize_t dst = elf->addr + elf->ehdr.e_phoff + elf->ehdr.e_phentsize * i;
		if (lseek(elf->fd, dst, SEEK_SET) != dst)
		{
			puts("failed to seek phdr\n");
			return 1;
		}
		if (read(elf->fd, &elf->phdr[i], sizeof(*elf->phdr)) != sizeof(*elf->phdr))
		{
			puts("failed to read phdr\n");
			return 1;
		}
	}

	elf->addr = 0x100000 + 0x100000 * g_elf_nb; /* XXX use current mapping */
	return 0;
}

static int elf_map(struct elf *elf)
{
	for (size_t i = 0; i < elf->ehdr.e_phnum; ++i)
	{
		const Elf32_Phdr *phdr = &elf->phdr[i];
		if (phdr->p_type != PT_LOAD)
			continue;
		if (handle_pt_load(elf, phdr))
			return 1;
	}
	return 0;
}

static int elf_resolve(struct elf *elf)
{
	for (size_t i = 0; i < elf->ehdr.e_phnum; ++i)
	{
		const Elf32_Phdr *phdr = &elf->phdr[i];
		if (phdr->p_type != PT_DYNAMIC)
			continue;
		if (handle_pt_dynamic(elf, phdr))
			return 1;
	}
	return 0;
}

static int main(int argc, char **argv, char **envp, void **jmp)
{
	int ret = 1;
	if (argc < 1)
	{
		puts("no binary given\n");
		return ret;
	}
	struct elf *elf = getelf();
	if (!elf)
		return ret;
	elf->fd = open(argv[0], O_RDONLY);
	if (elf->fd < 0)
	{
		puts("failed to open file\n");
		return ret;
	}
	if (elf_read(elf))
	{
		puts("failed to load elf\n");
		goto end;
	}
	if (elf_map(elf))
	{
		puts("failed to map elf\n");
		goto end;
	}
	if (elf_resolve(elf))
	{
		puts("failed to resolve elf\n");
		goto end;
	}
	*jmp = (void*)elf->ehdr.e_entry + elf->addr;
	ret = 0;

end:
	close(elf->fd);
	return ret;
}

typedef void (*jmp_t)(void);

void _start(int argc, char **argv, char **envp)
{
	g_stdout = open("/dev/tty0", O_RDONLY);
	if (g_stdout < 0)
		exit(1);
	puts("ld.so\n");
	void *jmp;
	int ret = main(argc, argv, envp, &jmp);
	puts("ld.so end\n");
	if (!ret)
		((jmp_t)jmp)();
	close(g_stdout);
	exit(ret);
}
