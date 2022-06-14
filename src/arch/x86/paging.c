#include "x86.h"
#include "sys/std.h"

#define DIR_MASK  0xFFC00000
#define TBL_MASK  0x003FF000
#define PGE_MASK  0x00000FFF
#define DIR_SHIFT 22
#define TBL_SHIFT 12
#define PGE_SHIFT 0

#define DIR_ID(addr) ((((uint32_t)(addr)) & DIR_MASK) >> DIR_SHIFT)
#define TBL_ID(addr) ((((uint32_t)(addr)) & TBL_MASK) >> TBL_SHIFT)
#define PGE_ID(addr) ((((uint32_t)(addr)) & PGE_MASK) >> PGE_SHIFT)

#define DIR_FLAG_P   (1 << 0) /* is present */
#define DIR_FLAG_RW  (1 << 1) /* enable write */
#define DIR_FLAG_US  (1 << 2) /* available for userspace */
#define DIR_FLAG_PWT (1 << 3)
#define DIR_FLAG_PCD (1 << 4) /* must not be cached */
#define DIR_FLAG_A   (1 << 5) /* has been accessed */

#define DIR_TBL_ADDR(addr) ((uint32_t)addr & 0xFFFFF000)

#define TBL_FLAG_P   (1 << 0) /* is present */
#define TBL_FLAG_RW  (1 << 1) /* enable write */
#define TBL_FLAG_US  (1 << 2) /* available for userspace */
#define TBL_FLAG_PWT (1 << 3)
#define TBL_FLAG_PCD (1 << 4) /* must not be cached */
#define TBL_FLAG_A   (1 << 5) /* has been accessed */
#define TBL_FLAG_D   (1 << 6) /* has been written to */
#define TBL_FLAG_PAT (1 << 7)
#define TBL_FLAG_G   (1 << 8) /* global page */
#define TBL_FLAG_V   (1 << 9) /* custom flag: virtual memory has been allocated */

#define TBL_VADDR_BASE ((uint32_t*)0xFFC00000)
#define TBL_VADDR(id)  (TBL_VADDR_BASE + (id) * 0x400)
#define DIR_VADDR      ((uint32_t*)0xFFFFF000)

#define PAGE_SIZE 0x1000
#define PAGE_MASK 0x0FFF

#define INVALIDATE_PAGE(page) __asm__ volatile ("invlpg (%0)" : : "a"(page));

static uint32_t g_kern_page_dirs[0x400] __attribute__((aligned(4096)));
static uint32_t g_kern_mirror[0x1000] __attribute__((aligned(4096))); /* 16MB mirror: 0x000000 - 0x1000000 */

static uint32_t *g_bitmap;
static uint32_t g_bitmap_size; /* of uint32_t */

static uint32_t g_addr; /* base memory addresss */
static uint32_t g_size; /* memory size */

static uint32_t alloc_page(void);

static inline uint32_t mkentry(uint32_t addr, uint32_t flags)
{
	return addr | flags;
}

static void init_mirror_pages(void)
{
	memset(g_kern_page_dirs, 0, sizeof(g_kern_page_dirs));
	g_kern_page_dirs[0] = mkentry((uint32_t)&g_kern_mirror[0x000], DIR_FLAG_P | DIR_FLAG_RW);
	g_kern_page_dirs[1] = mkentry((uint32_t)&g_kern_mirror[0x400], DIR_FLAG_P | DIR_FLAG_RW);
	g_kern_page_dirs[2] = mkentry((uint32_t)&g_kern_mirror[0x800], DIR_FLAG_P | DIR_FLAG_RW);
	g_kern_page_dirs[3] = mkentry((uint32_t)&g_kern_mirror[0xC00], DIR_FLAG_P | DIR_FLAG_RW);
	memset(g_kern_mirror, 0, sizeof(g_kern_mirror));
	for (size_t i = 0; i < PAGE_SIZE; ++i)
		g_kern_mirror[i] = (i * PAGE_SIZE) | 3;
}

static void init_recursive_pages(void)
{
	g_kern_page_dirs[DIR_ID(DIR_VADDR)] = mkentry((uint32_t)&g_kern_page_dirs[0], DIR_FLAG_P | DIR_FLAG_RW);
}

static void init_physical_maps(void)
{
	uint32_t pages_count = g_size / PAGE_SIZE;
	g_bitmap_size = (pages_count + 31) / 32;
	if (g_bitmap_size * 4 >= 0x1000 * 0x400) /* arbitrary: must not be greater than a single dir */
		panic("bitmap size too long: %lu / %u", g_bitmap_size * 4, 0x1000 * 0x400);
	g_bitmap = (uint32_t*)g_addr;
	memset(g_bitmap, 0, g_bitmap_size * sizeof(uint32_t));
	uint32_t bitmap_bytes = g_bitmap_size * 4;
	uint32_t bitmap_pages = (bitmap_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
	memset(g_bitmap, 0xFF, (bitmap_pages + 7) / 8);

	/* init bitmap tables page */
	uint32_t *tbl_page = (uint32_t*)alloc_page();
	uint32_t i;
	for (i = 0; i < (g_bitmap_size + 0x3FF) / 0x400; ++i)
		tbl_page[i] = mkentry((uint32_t)&g_bitmap[i * 0x400], TBL_FLAG_P | TBL_FLAG_RW);
	memset(&tbl_page[i], 0, PAGE_SIZE - 4 * i);
	g_kern_page_dirs[DIR_ID(g_bitmap)] = mkentry((uint32_t)&tbl_page[0], DIR_FLAG_P | DIR_FLAG_RW);
}

static void enable_paging(uint32_t *dirs)
{
	__asm__ volatile ("mov %0, %%cr3" : : "Nd"(dirs));
	__asm__ volatile ("mov %cr0, %eax; or $0x80000000, %eax; mov %eax, %cr0");
}

static void disable_paging(void)
{
	__asm__ volatile ("mov %cr0, %eax; and $0x7FFFFFFF, %eax; mov %eax, %cr0");
}

void paging_init(uint32_t addr, uint32_t size)
{
	g_addr = addr;
	g_size = size;
	printf("initializing memory %lx-%lx (%lu)\n", addr, addr + size, size);
	init_mirror_pages();
	init_physical_maps();
	init_recursive_pages();
	enable_paging(g_kern_page_dirs);
}

static uint32_t alloc_page(void)
{
	for (uint32_t i = 0; i < g_bitmap_size; ++i)
	{
		if (g_bitmap[i] == 0xFFFFFFFF)
			continue;
		uint32_t bitmap = g_bitmap[i];
		for (uint32_t j = 0; j < 32; ++j)
		{
			if (bitmap & (1 << j))
				continue;
			g_bitmap[i] |= (1 << j);
			return g_addr + (i * 32 + j) * PAGE_SIZE;
		}
	}
	panic("no page can be found\n");
	return 0;
}

static void free_page(uint32_t addr)
{
	if (addr < g_addr)
		panic("free_page of invalid address (too low)\n");
	if (addr >= g_addr + g_size)
		panic("free_page of invalid address (too high)\n");
	uint32_t delta = (addr - g_addr) / 0x1000;
	uint32_t *bitmap = &g_bitmap[delta / 32];
	uint32_t mask = (1 << delta % 32);
	if (!(*bitmap & mask))
		panic("free_page of unallocated page\n");
	*bitmap &= ~mask;
}

void paging_alloc(uint32_t addr)
{
	addr &= ~0xFFF;
	uint32_t dir_id = DIR_ID(addr);
	uint32_t dir = DIR_VADDR[dir_id];
	if (!(dir & DIR_FLAG_P))
		panic("paging non-allocated memory %lx\n", addr);
	uint32_t *tbl_ptr = TBL_VADDR(dir_id);
	uint32_t tbl_id = TBL_ID(addr);
	uint32_t tbl = tbl_ptr[tbl_id];
	if (tbl & TBL_FLAG_P)
		panic("allocating already present page %lx\n", addr);
	if (!(tbl & TBL_FLAG_V))
		panic("writing to non-allocated vmem tbl %lx\n", addr);
	tbl_ptr[tbl_id] = mkentry(alloc_page(), TBL_FLAG_RW | TBL_FLAG_P);
	INVALIDATE_PAGE(addr);
}

static void vmalloc_page(uint32_t addr)
{
	if (addr & 0xFFF)
		panic("alloc unaligned page %lx\n", addr);
	uint32_t dir_id = DIR_ID(addr);
	uint32_t dir = DIR_VADDR[dir_id];
	if (!(dir & DIR_FLAG_P))
	{
		dir = mkentry(alloc_page(), DIR_FLAG_RW | DIR_FLAG_P);
		DIR_VADDR[dir_id] = dir;
		memset(TBL_VADDR(dir_id), 0, PAGE_SIZE);
	}
	uint32_t *tbl_ptr = TBL_VADDR(dir_id);
	uint32_t tbl_id = TBL_ID(addr);
	uint32_t tbl = tbl_ptr[tbl_id];
	if (tbl & TBL_FLAG_P)
		panic("vmalloc already created page %lx\n", addr);
	if (tbl & TBL_FLAG_V)
		panic("vmalloc already allocated page %lx\n", addr);
	tbl_ptr[tbl_id] = mkentry(0, TBL_FLAG_V);
	INVALIDATE_PAGE(addr);
}

static void vfree_page(uint32_t addr)
{
	uint32_t dir_id = DIR_ID(addr);
	uint32_t dir = DIR_VADDR[dir_id];
	if (!(dir & DIR_FLAG_P))
		panic("free of unexisting dir\n");
	uint32_t *tbl_ptr = TBL_VADDR(dir_id);
	uint32_t tbl_id = TBL_ID(addr);
	uint32_t tbl = tbl_ptr[tbl_id];
	if (tbl & TBL_FLAG_P)
		free_page(DIR_TBL_ADDR(tbl));
	tbl_ptr[tbl_id] = mkentry(0, 0);
	INVALIDATE_PAGE(addr);
}

void paging_vmalloc(uint32_t addr, uint32_t size)
{
	if (addr & PAGE_MASK)
		panic("vmalloc unaligned data %lx\n", addr);
	if (size & PAGE_MASK)
		panic("vmalloc unaligned size %lx\n", size);
	for (uint32_t i = 0; i < size; i += PAGE_SIZE)
		vmalloc_page(addr + i);
}

void paging_vfree(uint32_t addr, uint32_t size)
{
	if (addr & PAGE_MASK)
		panic("vmalloc unaligned data %lx\n", addr);
	if (size & PAGE_MASK)
		panic("vmalloc unaligned size %lx\n", size);
}

void *vmalloc(size_t size)
{
	static uint32_t current_offset = 0x1800000UL; /* XXX: track available vaddr */
	if (size & PAGE_MASK)
		panic("malloc of unaligned size: %lx\n", size);
	uint32_t ret = current_offset;
	paging_vmalloc(ret, size);
	current_offset += size;
	return (void*)ret;
}

void vfree(void *ptr, size_t size)
{
	uint32_t addr = (uint32_t)ptr;
	if (addr & PAGE_MASK)
		panic("free of unaligned ptr: %p\n", ptr);
	if (size & PAGE_MASK)
		panic("free of unaligned size: %lx\n", size);
	for (uint32_t i = 0; i < size; i += PAGE_SIZE)
		vfree_page(addr + i);
}

void paging_dumpinfo()
{
	uint32_t pages_used = 0;
	for (uint32_t i = 0; i < g_bitmap_size; ++i)
	{
		uint32_t bitmap = g_bitmap[i];
		for (uint32_t j = 0; j < 32; ++j)
		{
			if (bitmap & (1 << j))
				pages_used++;
		}
	}
	printf("pages used: %lu\n", pages_used);
}
