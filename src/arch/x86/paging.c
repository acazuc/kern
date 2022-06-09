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

#define TBL_VADDR_BASE ((uint32_t*)0xFFC00000)
#define TBL_VADDR(id)  (TBL_VADDR_BASE + (id) * 0x400)
#define DIR_VADDR      ((uint32_t*)0xFFFFF000)

#define PAGE_SIZE 0x1000

static uint32_t g_kern_page_dirs[0x400] __attribute__((aligned(4096)));
static uint32_t g_kern_mirror[0x1000] __attribute__((aligned(4096))); /* 16MB mirror: 0x000000 - 0x1000000 */

static uint32_t *g_bitmap;
static uint32_t g_bitmap_size; /* of uint32_t */

static void *g_addr; /* base memory addresss */
static uint32_t g_size; /* memory size */

static void *alloc_page(void);

static inline uint32_t mkentry(void *addr, uint32_t flags)
{
	return (uint32_t)addr | flags;
}

static void init_mirror_pages(void)
{
	memset(g_kern_page_dirs, 0, sizeof(g_kern_page_dirs));
	g_kern_page_dirs[0] = mkentry(&g_kern_mirror[0x000], DIR_FLAG_P | DIR_FLAG_RW);
	g_kern_page_dirs[1] = mkentry(&g_kern_mirror[0x400], DIR_FLAG_P | DIR_FLAG_RW);
	g_kern_page_dirs[2] = mkentry(&g_kern_mirror[0x800], DIR_FLAG_P | DIR_FLAG_RW);
	g_kern_page_dirs[3] = mkentry(&g_kern_mirror[0xC00], DIR_FLAG_P | DIR_FLAG_RW);
	memset(g_kern_mirror, 0, sizeof(g_kern_mirror));
	for (size_t i = 0; i < PAGE_SIZE; ++i)
		g_kern_mirror[i] = (i * PAGE_SIZE) | 3;
}

static void init_recursive_pages(void)
{
	g_kern_page_dirs[DIR_ID(DIR_VADDR)] = mkentry(g_kern_page_dirs, DIR_FLAG_P | DIR_FLAG_RW);
}

static void init_physical_maps(void)
{
	uint32_t pages_count = g_size / PAGE_SIZE;
	g_bitmap_size = (pages_count + 31) / 32;
	if (g_bitmap_size * 4 >= 0x1000 * 0x400) /* arbitrary: must not be greater than a single dir */
	{
		printf("bitmap size too long: %lu / %u", g_bitmap_size * 4, 0x1000 * 0x400);
		panic();
	}
	g_bitmap = g_addr;
	memset(g_bitmap, 0, g_bitmap_size * sizeof(uint32_t));
	uint32_t bitmap_bytes = g_bitmap_size * 4;
	uint32_t bitmap_pages = (bitmap_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
	memset(g_bitmap, 0xFF, (bitmap_pages + 7) / 8);

	/* init bitmap tables page */
	uint32_t *tbl_page = alloc_page();
	uint32_t i;
	for (i = 0; i < (g_bitmap_size + 0x3FF) / 0x400; ++i)
		tbl_page[i] = mkentry(&g_bitmap[i * 0x400], TBL_FLAG_P | TBL_FLAG_RW);
	memset(&tbl_page[i], 0, PAGE_SIZE - 4 * i);
	g_kern_page_dirs[DIR_ID(g_bitmap)] = mkentry(tbl_page, DIR_FLAG_P | DIR_FLAG_RW);
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

void paging_init(void *addr, uint32_t size)
{
	g_addr = addr;
	g_size = size;
	printf("initializing memory %p-%p (%lu)\n", addr, addr + size, size);
	init_mirror_pages();
	init_physical_maps();
	init_recursive_pages();
	enable_paging(g_kern_page_dirs);
}

static void *alloc_page(void)
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
	printf("no page can be found\n");
	panic();
	return NULL;
}

void paging_alloc(void *addr)
{
	uint32_t dir_id = DIR_ID(addr);
	uint32_t dir = DIR_VADDR[dir_id];
	if (!(dir & DIR_FLAG_P))
	{
		uint32_t *page = alloc_page();
		dir = mkentry(page, DIR_FLAG_P | DIR_FLAG_RW);
		DIR_VADDR[dir_id] = dir;
		memset(TBL_VADDR(dir_id), 0, PAGE_SIZE);
	}
	uint32_t *tbl_ptr = TBL_VADDR(dir_id);
	uint32_t tbl_id = TBL_ID(addr);
	uint32_t tbl = tbl_ptr[tbl_id];
	if (tbl & TBL_FLAG_P)
	{
		printf("allocating already present page %08lx\n", (uint32_t)addr);
		panic();
	}
	uint32_t *page = alloc_page();
	tbl_ptr[tbl_id] = mkentry(page, TBL_FLAG_P | TBL_FLAG_RW);
}
