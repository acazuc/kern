#include "x86.h"
#include "sys/std.h"

/*
 * virtual memory layout
 *
 * 0x00000000 - 0xBFFFFFFF (3.2 GB): userland
 * 0xC0000000 - 0xC03FFFFF (4.0 MB): kernel binary
 * 0xC0400000 - 0xC07FFFFF (4.0 MB): physical bitmap
 * 0xC0800000 - 0xC0BFFFFF (4.0 MB): kern heap page tables
 * 0xC0C00000 - 0xFFBFFFFF (1.0 GB): kern heap
 * 0xFFC00000 - 0xFFFFFFFF (4.0 MB): recursive mapping
 */

#define VADDR_USER_BEG 0x00000000
#define VADDR_USER_END 0xC0000000
#define VADDR_KERN_BEG 0xC0000000
#define VADDR_KERN_END 0xC0400000
#define VADDR_PMBM_BEG 0xC0400000
#define VADDR_PMBM_END 0xC0800000
#define VADDR_KHPT_BEG 0xC0800000
#define VADDR_KHPT_END 0xC0C00000
#define VADDR_HEAP_BEG 0xC0C00000
#define VADDR_HEAP_END 0xFFC00000
#define VADDR_RECU_BEG 0xFFC00000
#define VADDR_RECU_END 0xFFFFFFFF /* should be 0x100000000 */

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

#define TBL_VADDR(id)  ((uint32_t*)VADDR_RECU_BEG + (id) * 0x400)
#define DIR_VADDR      ((uint32_t*)0xFFFFF000)

#define PAGE_MASK 0x0FFF

#define INVALIDATE_PAGE(page) __asm__ volatile ("invlpg (%0)" : : "a"(page) : "memory");

static uint32_t *g_pmm_bitmap;
static uint32_t g_pmm_bitmap_size; /* of uint32_t */
static uint32_t g_pmm_bitmap_first;
static uint32_t g_pmm_addr; /* base memory addresss */
static uint32_t g_pmm_size; /* memory size */

extern uint8_t _kernel_end;

static inline uint32_t mkentry(uint32_t addr, uint32_t flags)
{
	return addr | flags;
}

static uint32_t pmm_alloc_page(void)
{
	if (g_pmm_bitmap_first >= g_pmm_bitmap_size * 32)
		panic("no more pages available\n");
	uint32_t i = g_pmm_bitmap_first / 32;
	uint32_t j = g_pmm_bitmap_first % 32;
	if (g_pmm_bitmap[i] & (1 << j))
		panic("invalid first page\n");
	g_pmm_bitmap[i] |= (1 << j);
	uint32_t ret = g_pmm_addr + (i * 32 + j) * PAGE_SIZE;
	for (; i < g_pmm_bitmap_size; ++i)
	{
		if (g_pmm_bitmap[i] == 0xFFFFFFFF)
			continue;
		for (j = 0; j < 32; ++j)
		{
			if (!(g_pmm_bitmap[i] & (1 << j)))
			{
				g_pmm_bitmap_first = i * 32 + j;
				goto end;
			}
		}
		panic("no empty bits\n");
	}
end:
	return ret;
}

static void pmm_free_page(uint32_t addr)
{
	if (addr < g_pmm_addr)
		panic("free_page of invalid address (too low)\n");
	if (addr >= g_pmm_addr + g_pmm_size)
		panic("free_page of invalid address (too high)\n");
	uint32_t delta = (addr - g_pmm_addr) / PAGE_SIZE;
	uint32_t *bitmap = &g_pmm_bitmap[delta / 32];
	uint32_t mask = (1 << (delta % 32));
	if (!(*bitmap & mask))
		panic("free_page of unallocated page\n");
	*bitmap &= ~mask;
	if (delta < g_pmm_bitmap_first)
		g_pmm_bitmap_first = delta;
}

void paging_alloc(uint32_t addr)
{
	addr &= ~PAGE_MASK;
	uint32_t dir_id = DIR_ID(addr);
	uint32_t dir = DIR_VADDR[dir_id];
	if (!(dir & DIR_FLAG_P))
		panic("paging non-allocated memory 0x%lx\n", addr);
	uint32_t *tbl_ptr = TBL_VADDR(dir_id);
	uint32_t tbl_id = TBL_ID(addr);
	uint32_t tbl = tbl_ptr[tbl_id];
	if (tbl & TBL_FLAG_P)
		panic("allocating already present page 0x%lx\n", addr);
	if (!(tbl & TBL_FLAG_V))
		panic("writing to non-allocated vmem tbl 0x%lx\n", addr);
	tbl_ptr[tbl_id] = mkentry(pmm_alloc_page(), TBL_FLAG_RW | TBL_FLAG_P | TBL_FLAG_US); /* XXX: remove US */
}

static void vmm_alloc_page(uint32_t addr)
{
	if (addr & PAGE_MASK)
		panic("alloc unaligned page 0x%lx\n", addr);
	uint32_t dir_id = DIR_ID(addr);
	uint32_t dir = DIR_VADDR[dir_id];
	if (!(dir & DIR_FLAG_P))
	{
		dir = mkentry(pmm_alloc_page(), DIR_FLAG_RW | DIR_FLAG_P | DIR_FLAG_US); /* XXX: remove US */
		DIR_VADDR[dir_id] = dir;
		memset(TBL_VADDR(dir_id), 0, PAGE_SIZE);
	}
	uint32_t *tbl_ptr = TBL_VADDR(dir_id);
	uint32_t tbl_id = TBL_ID(addr);
	uint32_t tbl = tbl_ptr[tbl_id];
	if (tbl & TBL_FLAG_P)
		panic("vmalloc already created page 0x%lx\n", addr);
	if (tbl & TBL_FLAG_V)
		panic("vmalloc already allocated page 0x%lx\n", addr);
	tbl_ptr[tbl_id] = mkentry(0, TBL_FLAG_V);
	INVALIDATE_PAGE(addr);
}

static void vmm_free_page(uint32_t addr)
{
	uint32_t dir_id = DIR_ID(addr);
	uint32_t dir = DIR_VADDR[dir_id];
	if (!(dir & DIR_FLAG_P))
		panic("free of unexisting dir\n");
	uint32_t *tbl_ptr = TBL_VADDR(dir_id);
	uint32_t tbl_id = TBL_ID(addr);
	uint32_t tbl = tbl_ptr[tbl_id];
	if (tbl & TBL_FLAG_P)
		pmm_free_page(DIR_TBL_ADDR(tbl));
	tbl_ptr[tbl_id] = mkentry(0, 0);
	INVALIDATE_PAGE(addr);
}

typedef struct vmm_free_range vmm_free_range_t;

struct vmm_free_range
{
	uint32_t addr;
	uint32_t size;
	vmm_free_range_t *prev;
	vmm_free_range_t *next;
};

typedef struct vmm_free_ctx_s
{
	uint32_t addr;
	uint32_t size;
	vmm_free_range_t free_range_0; /* always have an available item */
	vmm_free_range_t *free_ranges; /* address-ordered */
} vmm_free_ctx_t;

static vmm_free_ctx_t g_vmm_heap_free_ctx; /* kernel heap */

static uint32_t vmm_get_free_range(vmm_free_ctx_t *ctx, uint32_t size)
{
	if (!ctx->free_ranges)
	{
		ctx->free_ranges = &ctx->free_range_0;
		ctx->free_range_0.prev = NULL;
		ctx->free_range_0.next = NULL;
		ctx->free_range_0.addr = ctx->addr + size;
		ctx->free_range_0.size = ctx->size - size;
		return ctx->addr;
	}
	for (vmm_free_range_t *item = ctx->free_ranges; item; item = item->next)
	{
		if (item->size >= size)
		{
			uint32_t addr = item->addr;
			item->addr += size;
			item->size -= size;
			return addr;
		}
	}
	return UINT32_MAX;
}

static void vmm_set_free_range(vmm_free_ctx_t *ctx, uint32_t addr, uint32_t size)
{
	if (!ctx->free_ranges)
	{
		ctx->free_ranges = &ctx->free_range_0;
		ctx->free_range_0.prev = NULL;
		ctx->free_range_0.next = NULL;
		ctx->free_range_0.addr = addr;
		ctx->free_range_0.size = size;
		return;
	}
	for (vmm_free_range_t *item = ctx->free_ranges; ; item = item->next)
	{
		if (item->addr == addr + size)
		{
			item->addr -= size;
			item->size += size;
			if (item->prev)
			{
				if (item->prev->addr + size == addr)
				{
					item->prev->size += item->size;
					item->prev->next = item->next;
					if (item->next)
						item->next->prev = item->prev;
					if (item != &ctx->free_range_0)
						free(item);
				}
			}
			return;
		}
		if (item->addr + item->size == addr)
		{
			item->size += size;
			if (item->next)
			{
				if (item->next->addr == item->addr + item->size)
				{
					item->size += item->next->size;
					if (item->next->next)
						item->next->next->prev = item;
					vmm_free_range_t *next = item->next->next;
					if (item->next != &ctx->free_range_0)
						free(item->next);
					item->next = next;
				}
			}
			return;
		}
		if (addr < item->addr)
		{
			vmm_free_range_t *new = malloc(sizeof(*new), 0);
			if (!new)
				panic("can't allocate new free space\n");
			new->next = item;
			new->prev = item->prev;
			item->prev = new;
			if (new->prev)
				new->prev->next = new;
			else
				ctx->free_ranges = new;
			new->addr = addr;
			new->size = size;
			return;
		}
		if (!item->next)
		{
			vmm_free_range_t *new = malloc(sizeof(*new), 0);
			if (!new)
				panic("can't allocate new free space\n");
			new->next = NULL;
			new->prev = item;
			item->next = new;
			new->addr = addr;
			new->size = size;
			return;
		}
	}
	panic("dead code\n");
}

void *vmalloc(size_t size)
{
	if (size & PAGE_MASK)
		panic("vmalloc unaligned size 0x%lx\n", size);
	uint32_t addr = vmm_get_free_range(&g_vmm_heap_free_ctx, size);
	if (addr == UINT32_MAX)
		return NULL;
	if (addr & PAGE_MASK)
		panic("vmalloc unaligned data 0x%lx\n", addr);
	for (uint32_t i = 0; i < size; i += PAGE_SIZE)
		vmm_alloc_page(addr + i);
	return (void*)addr;
}

void vfree(void *ptr, size_t size)
{
	uint32_t addr = (uint32_t)ptr;
	if (addr & PAGE_MASK)
		panic("free of unaligned addr: 0x%lx\n", addr);
	if (size & PAGE_MASK)
		panic("free of unaligned size: 0x%lx\n", size);
	vmm_set_free_range(&g_vmm_heap_free_ctx, addr, size);
	for (uint32_t i = 0; i < size; i += PAGE_SIZE)
		vmm_free_page(addr + i);
}

static void init_physical_maps(void)
{
	uint32_t pages_count = g_pmm_size / PAGE_SIZE;
	g_pmm_bitmap_size = (pages_count + 31) / 32;
	if (g_pmm_bitmap_size * 32 >= PAGE_SIZE * 0x400) /* arbitrary: must not be greater than a single dir */
		panic("bitmap size too long: %lu / %u", g_pmm_bitmap_size * 4, PAGE_SIZE * 0x400);

	uint32_t bitmap_bytes = g_pmm_bitmap_size * sizeof(uint32_t);
	uint32_t bitmap_pages = (bitmap_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
	uint32_t addr = (uint32_t)&_kernel_end;
	addr += PAGE_SIZE * 1024 - 1;
	addr -= addr % (PAGE_SIZE * 1024);
	g_pmm_bitmap = (uint32_t*)addr;

	uint32_t dir_id = DIR_ID(g_pmm_bitmap);
	uint32_t *dir_ptr = &DIR_VADDR[dir_id];
	*dir_ptr = mkentry(g_pmm_addr, DIR_FLAG_P | DIR_FLAG_RW | DIR_FLAG_US); /* XXX: remove US */
	uint32_t *tbl_ptr = TBL_VADDR(dir_id);
	uint32_t tbl_id = TBL_ID(g_pmm_bitmap);
	for (uint32_t i = 0; i < bitmap_pages; ++i)
		tbl_ptr[tbl_id + i] = mkentry(g_pmm_addr + PAGE_SIZE + i * PAGE_SIZE, TBL_FLAG_P | TBL_FLAG_RW | TBL_FLAG_US); /* XXX: remove US */

	memset(g_pmm_bitmap, 0, bitmap_bytes);
	for (size_t i = 0; i <= bitmap_pages; ++i)
		g_pmm_bitmap[i / 32] |= (1 << (i % 32));
	g_pmm_bitmap_first = bitmap_pages + 1;
}

static void init_heap_pages_tables(void)
{
	for (uint32_t addr = VADDR_HEAP_BEG; addr < VADDR_HEAP_END; addr += 0x400000)
	{
		uint32_t dir_id = DIR_ID(addr);
		uint32_t *dir = &DIR_VADDR[dir_id];
		if (*dir != 0)
			panic("non-NULL heap page table 0x%lx\n", addr);
		*dir = mkentry(pmm_alloc_page(), DIR_FLAG_P | DIR_FLAG_RW | DIR_FLAG_US); /* XXX: remove US */
		uint32_t *tbl_ptr = TBL_VADDR(dir_id);
		memset(tbl_ptr, 0, 0x1000);
	}
}

void paging_init(uint32_t addr, uint32_t size)
{
	g_pmm_addr = addr;
	g_pmm_size = size;
	printf("initializing memory 0x%lx-0x%lx (%lu)\n", addr, addr + size, size);
	init_physical_maps();
	init_heap_pages_tables();
	g_vmm_heap_free_ctx.addr = VADDR_HEAP_BEG;
	g_vmm_heap_free_ctx.size = VADDR_HEAP_END - VADDR_HEAP_BEG;
	g_vmm_heap_free_ctx.free_ranges = NULL;
}

static void pmm_dumpinfo(void)
{
	uint32_t pages_used = 0;
	for (uint32_t i = 0; i < g_pmm_bitmap_size; ++i)
	{
		uint32_t bitmap = g_pmm_bitmap[i];
		if (!bitmap)
			continue;
		if (bitmap == 0xFFFFFFFF)
		{
			pages_used += 32;
			continue;
		}
		for (uint32_t j = 0; j < 32; ++j)
		{
			if (bitmap & (1 << j))
				pages_used++;
		}
	}
	printf("pages used: 0x%05lx / 0x%05lx (%d%%); first available: %05lx\n", pages_used, g_pmm_bitmap_size * 32, (int)(pages_used / (float)(g_pmm_bitmap_size * 32) * 100), g_pmm_bitmap_first);
}

static void vmm_dumpinfo(void)
{
	for (vmm_free_range_t *item = g_vmm_heap_free_ctx.free_ranges; item; item = item->next)
	{
		printf("0x%lx - 0x%lx: 0x%lx bytes\n", item->addr, item->addr + item->size, item->size);
	}
}

void paging_dumpinfo()
{
	pmm_dumpinfo();
	vmm_dumpinfo();
}
