#include "x86.h"
#include "asm.h"

#include <sys/proc.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

/*
 * physical memory layout
 *
 * 0x00000000 - 0x0009FFFF (640 KB): main memory
 * 0x000A0000 - 0x000BFFFF (128 KB): display buffer
 * 0x000C0000 - 0x000DFFFF (128 KB): ROM BIOS for addon cards
 * 0x000E0000 - 0x000FFFFF (128 KB): system ROM BIOS
 * 0x00100000 - 0xFEBFFFFF (4.2 GB): main memory
 * 0xFEC00000 - 0xFECFFFFF (1.0 MB): APIC IO unit
 * 0xFED00000 - 0xFEDFFFFF (1.0 MB): memory-mapped IO devices
 * 0xFEE00000 - 0xFEEFFFFF (1.0 MB): APIC local unit
 * 0xFEF00000 - 0xFFFDFFFF (1.0 MB): memory-mapped IO devices
 * 0xFFFE0000 - 0xFFFFFFFF (128 KB): init ROM
 */

/*
 * virtual memory layout
 *
 * 0x00000000 - 0x000FFFFF (1.0 MB): unused
 * 0x00100000 - 0xBFFFFFFF (3.2 GB): userland
 * 0xC0000000 - 0xC03FFFFF (4.0 MB): kernel binary
 * 0xC0400000 - 0xC07FFFFF (4.0 MB): physical bitmap
 * 0xC0800000 - 0xC0BFFFFF (4.0 MB): kern heap page tables
 * 0xC0C00000 - 0xFFBFFFFF (1.0 GB): kern heap
 * 0xFFC00000 - 0xFFFFFFFF (4.0 MB): recursive mapping
 */

/*
 * IMPORTANT: assumption is made that the all the directory
 * pages for all the process will always be allocated mapped
 * in the kernel heap region
 *
 * all the table pages from the kernel memory are pre-allocated
 * and are always visible
 */

#define VADDR_USER_BEG 0x00100000
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

#define DIR_FLAG_P    (1 << 0) /* is present */
#define DIR_FLAG_RW   (1 << 1) /* enable write */
#define DIR_FLAG_US   (1 << 2) /* available for userspace */
#define DIR_FLAG_PWT  (1 << 3)
#define DIR_FLAG_PCD  (1 << 4) /* must not be cached */
#define DIR_FLAG_A    (1 << 5) /* has been accessed */
#define DIR_FLAG_MASK 0xFFF

#define DIR_TBL_ADDR(addr) ((uint32_t)addr & 0xFFFFF000)

#define TBL_FLAG_P    (1 << 0) /* is present */
#define TBL_FLAG_RW   (1 << 1) /* enable write */
#define TBL_FLAG_US   (1 << 2) /* available for userspace */
#define TBL_FLAG_PWT  (1 << 3)
#define TBL_FLAG_PCD  (1 << 4) /* must not be cached */
#define TBL_FLAG_A    (1 << 5) /* has been accessed */
#define TBL_FLAG_D    (1 << 6) /* has been written to */
#define TBL_FLAG_PAT  (1 << 7)
#define TBL_FLAG_G    (1 << 8) /* global page */
#define TBL_FLAG_V    (1 << 9) /* custom flag: virtual memory has been allocated */
#define TBL_FLAG_MASK 0xFFF

#define TBL_VADDR(id)  ((uint32_t*)VADDR_RECU_BEG + (id) * 0x400)
#define DIR_VADDR      ((uint32_t*)0xFFFFF000)

#define PAGE_MASK 0x0FFF

struct vmm_range
{
	uint32_t addr;
	uint32_t size;
	TAILQ_ENTRY(vmm_range) chain;
};

TAILQ_HEAD(vmm_range_head, vmm_range);

struct vmm_region
{
	uint32_t addr;
	uint32_t size;
	struct vmm_range range_0; /* always have an available item */
	struct vmm_range_head ranges; /* address-ordered */
};

struct vmm_ctx
{
	uint32_t dir[1024]; /* must be at the top for page alignment */
	struct vmm_region region;
	uint32_t dir_paddr;
};

static struct vmm_region g_vmm_heap; /* kernel heap */

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

static uint32_t get_paddr(uint32_t vaddr)
{
	uint32_t *tbl = TBL_VADDR(DIR_ID(vaddr));
	uint32_t page = tbl[TBL_ID(vaddr)] & ~PAGE_MASK;
	return page | PGE_ID(vaddr);
}

static uint32_t pmm_alloc_page(void)
{
	assert(g_pmm_bitmap_first < g_pmm_bitmap_size * 32, "no more pages available\n");
	uint32_t i = g_pmm_bitmap_first / 32;
	uint32_t j = g_pmm_bitmap_first % 32;
	assert(!(g_pmm_bitmap[i] & (1 << j)), "invalid first page\n");
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
	assert(addr >= g_pmm_addr, "free_page of invalid address (too low): %lx\n", addr);
	assert(addr < g_pmm_addr + g_pmm_size, "free_page of invalid address (too high): %lx\n", addr);
	uint32_t delta = (addr - g_pmm_addr) / PAGE_SIZE;
	uint32_t *bitmap = &g_pmm_bitmap[delta / 32];
	uint32_t mask = (1 << (delta % 32));
	assert(*bitmap & mask, "free_page of unallocated page: %lx\n", addr);
	*bitmap &= ~mask;
	if (delta < g_pmm_bitmap_first)
		g_pmm_bitmap_first = delta;
}

void paging_alloc(uint32_t addr)
{
	addr &= ~PAGE_MASK;
	struct vmm_ctx *ctx = curproc ? curproc->vmm_ctx : NULL;
	uint32_t dir_id = DIR_ID(addr);
	uint32_t dir = ctx ? ctx->dir[dir_id] : DIR_VADDR[dir_id];
	assert(dir & DIR_FLAG_P, "paging non-allocated memory 0x%lx\n", addr);
	uint32_t *tbl_ptr = vmap(dir & ~PAGE_MASK, PAGE_SIZE);
	assert(tbl_ptr, "can't map tbl\n");
	uint32_t tbl_id = TBL_ID(addr);
	uint32_t *tbl = &tbl_ptr[tbl_id];
	assert(!(*tbl & TBL_FLAG_P), "allocating already present page 0x%lx\n", addr);
	assert(*tbl & TBL_FLAG_V, "writing to non-allocated vmem tbl 0x%lx\n", addr);
	uint32_t f = TBL_FLAG_RW | TBL_FLAG_P;
	if (*tbl & TBL_FLAG_US)
		f |= TBL_FLAG_US;
	*tbl = mkentry(pmm_alloc_page(), f);
	vunmap(tbl_ptr, PAGE_SIZE);
}

static void vmm_alloc_page(struct vmm_ctx *ctx, uint32_t addr, int user)
{
	assert(!(addr & PAGE_MASK), "alloc unaligned page 0x%lx\n", addr);
	uint32_t dir_id = DIR_ID(addr);
	uint32_t *dir = ctx ? &ctx->dir[dir_id] : &DIR_VADDR[dir_id];
	uint32_t *tbl_ptr;
	if (!(*dir & DIR_FLAG_P))
	{
		uint32_t f = DIR_FLAG_RW | DIR_FLAG_P;
		if (user)
			f |= DIR_FLAG_US;
		uint32_t paddr = pmm_alloc_page();
		*dir = mkentry(paddr, f);
		tbl_ptr = vmap(paddr, PAGE_SIZE);
		assert(tbl_ptr, "can't vmap tbl\n");
		memset(tbl_ptr, 0, PAGE_SIZE);
	}
	else
	{
		tbl_ptr = vmap(*dir & ~PAGE_MASK, PAGE_SIZE);
		assert(tbl_ptr, "can't map tbl\n");
	}
	uint32_t tbl_id = TBL_ID(addr);
	uint32_t *tbl = &tbl_ptr[tbl_id];
	assert(!(*tbl & TBL_FLAG_P), "vmalloc already created page 0x%lx\n", addr);
	assert(!(*tbl & TBL_FLAG_V), "vmalloc already allocated page 0x%lx\n", addr);
	uint32_t f = TBL_FLAG_V;
	if (user)
		f |= TBL_FLAG_US;
	*tbl = mkentry(0, f);
	vunmap(tbl_ptr, PAGE_SIZE);
	invlpg(addr);
}

static void vmm_free_page(struct vmm_ctx *ctx, uint32_t addr)
{
	uint32_t dir_id = DIR_ID(addr);
	uint32_t *dir = ctx ? &ctx->dir[dir_id] : &DIR_VADDR[dir_id];
	assert(*dir & DIR_FLAG_P, "free of unexisting dir\n");
	uint32_t *tbl_ptr = vmap(*dir & ~PAGE_MASK, PAGE_SIZE);
	assert(tbl_ptr, "can't map tbl\n");
	uint32_t tbl_id = TBL_ID(addr);
	uint32_t *tbl = &tbl_ptr[tbl_id];
	if (*tbl & TBL_FLAG_P)
		pmm_free_page(*tbl & ~PAGE_MASK);
	*tbl = mkentry(0, 0);
	vunmap(tbl_ptr, PAGE_SIZE);
	invlpg(addr);
}

static void vmm_map_page(uint32_t addr, uint32_t paddr)
{
	assert(!(addr & PAGE_MASK), "vmap unaligned page 0x%lx\n", addr);
	uint32_t dir_id = DIR_ID(addr);
	uint32_t dir = DIR_VADDR[dir_id];
	if (!(dir & DIR_FLAG_P))
	{
		dir = mkentry(pmm_alloc_page(), DIR_FLAG_RW | DIR_FLAG_P);
		DIR_VADDR[dir_id] = dir;
		memset(TBL_VADDR(dir_id), 0, PAGE_SIZE);
	}
	uint32_t *tbl_ptr = TBL_VADDR(dir_id);
	uint32_t tbl_id = TBL_ID(addr);
	uint32_t tbl = tbl_ptr[tbl_id];
	assert(!(tbl & TBL_FLAG_P), "vmap already created page 0x%lx\n", addr);
	assert(!(tbl & TBL_FLAG_V), "vmap already allocated page 0x%lx\n", addr);
	tbl_ptr[tbl_id] = mkentry(paddr, TBL_FLAG_RW | TBL_FLAG_P);
	invlpg(addr);
}

static void vmm_unmap_page(uint32_t addr)
{
	uint32_t dir_id = DIR_ID(addr);
	uint32_t dir = DIR_VADDR[dir_id];
	assert(dir & DIR_FLAG_P, "free of unexisting dir\n");
	uint32_t *tbl_ptr = TBL_VADDR(dir_id);
	uint32_t tbl_id = TBL_ID(addr);
	uint32_t tbl = tbl_ptr[tbl_id];
	assert(tbl & TBL_FLAG_P, "unmap non-mapped page 0x%08lx\n", addr);
	tbl_ptr[tbl_id] = mkentry(0, 0);
	invlpg(addr);
}

/* that's a bit of sad code, it somehow should be simplified */
static uint32_t vmm_get_free_range(struct vmm_region *region, uint32_t addr, uint32_t size)
{
	if (addr && (addr < region->addr || addr + size > region->addr + region->size))
		return UINT32_MAX;
	if (TAILQ_EMPTY(&region->ranges))
	{
		if (!addr || addr == region->addr)
		{
			region->range_0.addr = region->addr + size;
			region->range_0.size = region->size - size;
			TAILQ_INSERT_HEAD(&region->ranges, &region->range_0, chain);
			return region->addr;
		}
		if (addr + size == region->addr + region->size)
		{
			region->range_0.addr = region->addr;
			region->range_0.size = region->size - size;
			TAILQ_INSERT_HEAD(&region->ranges, &region->range_0, chain);
			return region->addr + region->size - size;
		}
		struct vmm_range *range = malloc(sizeof(*range), 0);
		assert(range, "can't allocate new range\n");
		range->addr = region->addr;
		range->size = addr - region->addr;
		TAILQ_INSERT_HEAD(&region->ranges, range, chain);
		region->range_0.addr = addr + size;
		region->range_0.size = region->addr + region->size - region->range_0.addr;
		TAILQ_INSERT_HEAD(&region->ranges, &region->range_0, chain);
		return addr;
	}
	struct vmm_range *item;
	if (addr)
	{
		TAILQ_FOREACH(item, &region->ranges, chain)
		{
			if (addr < item->addr)
				return UINT32_MAX;
			if (addr >= item->addr + item->size)
				continue;
			if (size > item->size)
				continue;
			if (item->size == size)
			{
				if (item->addr != addr)
					continue;
				TAILQ_REMOVE(&region->ranges, item, chain);
				if (item != &region->range_0)
					free(item);
				return addr;
			}
			if (addr == item->addr)
			{
				item->addr += size;
				item->size -= size;
				return addr;
			}
			if (addr + size == item->addr + item->size)
			{
				item->size -= size;
				return addr;
			}
			struct vmm_range *newr = malloc(sizeof(*newr), 0);
			assert(newr, "can't allocate new range\n");
			newr->addr = addr + size;
			newr->size = item->size - (addr + size - item->addr);
			TAILQ_INSERT_AFTER(&region->ranges, item, newr, chain);
			item->size = addr - item->addr;
			return addr;
		}
	}
	else
	{
		TAILQ_FOREACH(item, &region->ranges, chain)
		{
			if (item->size < size)
				continue;
			uint32_t addr = item->addr;
			if (item->size == size)
			{
				TAILQ_REMOVE(&region->ranges, item, chain);
				if (item != &region->range_0)
					free(item);
			}
			else
			{
				item->addr += size;
				item->size -= size;
			}
			return addr;
		}
	}
	return UINT32_MAX;
}

/* same sad code sa above */
static void vmm_set_free_range(struct vmm_region *region, uint32_t addr, uint32_t size)
{
	if (TAILQ_EMPTY(&region->ranges))
	{
		region->range_0.addr = addr;
		region->range_0.size = size;
		TAILQ_INSERT_HEAD(&region->ranges, &region->range_0, chain);
		return;
	}
	struct vmm_range *item;
	TAILQ_FOREACH(item, &region->ranges, chain)
	{
		if (item->addr == addr + size)
		{
			item->addr -= size;
			item->size += size;
			struct vmm_range *prev = TAILQ_PREV(item, vmm_range_head, chain);
			if (!prev)
				return;
			if (prev->addr + size != addr)
				return;
			prev->size += item->size;
			TAILQ_REMOVE(&region->ranges, item, chain);
			if (item != &region->range_0)
				free(item);
			return;
		}
		if (item->addr + item->size == addr)
		{
			item->size += size;
			struct vmm_range *next = TAILQ_NEXT(item, chain);
			if (!next)
				return;
			if (next->addr != item->addr + item->size)
				return;
			item->size += next->size;
			TAILQ_REMOVE(&region->ranges, next, chain);
			if (next != &region->range_0)
				free(next);
			return;
		}
		if (addr < item->addr)
		{
			struct vmm_range *new = malloc(sizeof(*new), 0);
			assert(new, "can't allocate new free space\n");
			new->addr = addr;
			new->size = size;
			TAILQ_INSERT_BEFORE(item, new, chain);
			return;
		}
	}
	struct vmm_range *new = malloc(sizeof(*new), 0);
	assert(new, "can't allocate new free space\n");
	new->addr = addr;
	new->size = size;
	TAILQ_INSERT_TAIL(&region->ranges, new, chain);
}

void vmm_setctx(const struct vmm_ctx *ctx)
{
	setcr3(ctx->dir_paddr);
}

static struct vmm_ctx *alloc_vmm_ctx(void)
{
	struct vmm_ctx *ctx;
	size_t size = sizeof(*ctx) + PAGE_SIZE - 1;
	size -= size % PAGE_SIZE;
	return vmalloc(size);
}

struct vmm_ctx *create_vmm_ctx(void)
{
	struct vmm_ctx *ctx = alloc_vmm_ctx();
	assert(ctx, "can't allocate new vmm ctx\n");
	memset(ctx->dir, 0, 768 * sizeof(uint32_t));
	for (size_t i = 768; i < 1024; ++i)
		ctx->dir[i] = DIR_VADDR[i];
	ctx->region.addr = VADDR_USER_BEG;
	ctx->region.size = VADDR_USER_END - VADDR_USER_BEG;
	ctx->dir_paddr = get_paddr((uint32_t)ctx->dir); /* must be mapped before getting addr */
	TAILQ_INIT(&ctx->region.ranges);
	return ctx;
}

/* XXX: to be removed */
static void vmm_dup_page(struct vmm_ctx *dst, const struct vmm_ctx *src, uint32_t addr)
{
	/* XXX: real way to do it ? */
	uint32_t dir_id = DIR_ID(addr);
	const uint32_t *dir_src = src ? &src->dir[dir_id] : &DIR_VADDR[dir_id];
	uint32_t *dir_dst = dst ? &dst->dir[dir_id] : &DIR_VADDR[dir_id];
	*dir_dst = *dir_src;
	if (*dir_src & DIR_FLAG_P)
	{
		uint32_t tbl_id = TBL_ID(addr);
		uint32_t *tbl_src = vmap(*dir_src & ~PAGE_MASK, PAGE_SIZE);
		uint32_t *tbl_dst = vmap(*dir_dst & ~PAGE_MASK, PAGE_SIZE);
		assert(tbl_src, "can't map tbl src\n");
		assert(tbl_dst, "can't map tbl dst\n");
		tbl_dst[tbl_id] = tbl_src[tbl_id];
		vunmap(tbl_src, PAGE_SIZE);
		vunmap(tbl_dst, PAGE_SIZE);
	}
}

void vmm_dup(struct vmm_ctx *dst, const struct vmm_ctx *src, uint32_t addr, uint32_t size)
{
	assert(!(addr & PAGE_MASK), "dup unaligned addr: 0x%lx\n", addr);
	assert(!(size & PAGE_MASK), "dup unaligned size: 0x%lx\n", size);
	for (uint32_t i = 0; i < size; i += PAGE_SIZE)
		vmm_dup_page(dst, src, addr + i);
}

static void dup_region(struct vmm_region *dst, const struct vmm_region *src)
{
	dst->addr = src->addr;
	dst->size = src->size;
	TAILQ_INIT(&dst->ranges);
	struct vmm_range *item;
	TAILQ_FOREACH(item, &src->ranges, chain)
	{
		struct vmm_range *newr = malloc(sizeof(*newr), 0);
		assert(newr, "can't duplicate new range\n");
		newr->addr = item->addr;
		newr->size = item->size;
		TAILQ_INSERT_TAIL(&dst->ranges, newr, chain);
	}
}

struct vmm_ctx *vmm_ctx_dup(const struct vmm_ctx *ctx)
{
	struct vmm_ctx *dup = alloc_vmm_ctx();
	assert(dup, "can't allocate dup vmm ctx\n");
	dup_region(&dup->region, &ctx->region);
	for (size_t i = 0; i < 768; ++i)
	{
		if (!(ctx->dir[i] & DIR_FLAG_P))
		{
			dup->dir[i] = ctx->dir[i];
			continue;
		}
		dup->dir[i] = mkentry(pmm_alloc_page(), ctx->dir[i] & DIR_FLAG_MASK);
		uint32_t *tbl_dst = vmap(dup->dir[i] & ~PAGE_MASK, PAGE_SIZE);
		uint32_t *tbl_src = vmap(ctx->dir[i] & ~PAGE_MASK, PAGE_SIZE);
		assert(tbl_dst, "can't map dst tbl\n");
		assert(tbl_src, "can't map src tbl\n");
		for (size_t j = 0; j < 1024; ++j)
		{
			if (!(tbl_src[j] & TBL_FLAG_P))
			{
				tbl_dst[j] = tbl_src[j];
				continue;
			}
			tbl_dst[j] = mkentry(pmm_alloc_page(), tbl_src[j] & TBL_FLAG_MASK);
			uint32_t *pge_dst = vmap(tbl_dst[j] & ~PAGE_MASK, PAGE_SIZE);
			uint32_t *pge_src = vmap(tbl_src[j] & ~PAGE_MASK, PAGE_SIZE);
			assert(pge_dst, "can't map dst page\n");
			assert(pge_src, "can't map src page\n");
			memcpy(pge_dst, pge_src, PAGE_SIZE);
			vunmap(pge_dst, PAGE_SIZE);
			vunmap(pge_src, PAGE_SIZE);
		}
		vunmap(tbl_src, PAGE_SIZE);
		vunmap(tbl_dst, PAGE_SIZE);
	}
	for (size_t i = 768; i < 1024; ++i)
		dup->dir[i] = ctx->dir[i];
	dup->dir_paddr = get_paddr((uint32_t)dup->dir); /* must be mapped before getting addr */
	return dup;
}

static void *vmalloc_zone(struct vmm_ctx *ctx, void *ptr, size_t size, int user)
{
	uint32_t addr = (uint32_t)ptr;
	assert(!(addr & PAGE_MASK), "vmalloc unaligned addr 0x%lx\n", addr);
	assert(!(size & PAGE_MASK), "vmalloc unaligned size 0x%lx\n", size);
	addr = vmm_get_free_range(ctx ? &ctx->region : &g_vmm_heap, addr, size);
	if (addr == UINT32_MAX)
		return NULL;
	assert(!(addr & PAGE_MASK), "vmalloc unaligned data 0x%lx\n", addr);
	for (uint32_t i = 0; i < size; i += PAGE_SIZE)
		vmm_alloc_page(ctx, addr + i, user);
	return (void*)addr;
}

static void vfree_zone(struct vmm_ctx *ctx, void *ptr, size_t size)
{
	uint32_t addr = (uint32_t)ptr;
	assert(!(addr & PAGE_MASK), "free of unaligned addr: 0x%lx\n", addr);
	assert(!(size & PAGE_MASK), "free of unaligned size: 0x%lx\n", size);
	for (uint32_t i = 0; i < size; i += PAGE_SIZE)
		vmm_free_page(ctx, addr + i);
	vmm_set_free_range(ctx ? &ctx->region : &g_vmm_heap, addr, size);
}

void *vmalloc(size_t size)
{
	return vmalloc_zone(NULL, NULL, size, 0);
}

void vfree(void *ptr, size_t size)
{
	vfree_zone(NULL, ptr, size);
}

void *vmalloc_user(struct vmm_ctx *ctx, void *addr, size_t size)
{
	return vmalloc_zone(ctx, addr, size, 1);
}

void vfree_user(struct vmm_ctx *ctx, void *ptr, size_t size)
{
	vfree_zone(ctx, ptr, size);
}

void *vmap(size_t paddr, size_t size)
{
	assert(!(paddr & PAGE_MASK), "vmap unaligned addr 0x%lx\n", paddr);
	assert(!(size  & PAGE_MASK), "vmap unaligned size 0x%lx\n", size);
	uint32_t addr = vmm_get_free_range(&g_vmm_heap, 0, size);
	if (addr == UINT32_MAX)
		return NULL;
	assert(!(addr & PAGE_MASK), "vmap unaligned data 0x%lx\n", addr);
	for (uint32_t i = 0; i < size; i += PAGE_SIZE)
		vmm_map_page(addr + i, paddr + i);
	return (void*)addr;
}

void *vmap_user(struct vmm_ctx *ctx, void *ptr, size_t size)
{
	uint32_t addr = (uint32_t)ptr;
	assert(!(addr & PAGE_MASK), "vmap unaligned addr 0x%lx\n", addr);
	assert(!(size  & PAGE_MASK), "vmap unaligned size 0x%lx\n", size);
	/* XXX */
	return NULL;
}

void vunmap(void *ptr, size_t size)
{
	uint32_t addr = (uint32_t)ptr;
	assert(!(addr & PAGE_MASK), "vunmap unaligned addr 0x%lx\n", addr);
	assert(!(size & PAGE_MASK), "vunmap unaligned size 0x%lx\n", size);
	vmm_set_free_range(&g_vmm_heap, addr, size);
	for (uint32_t i = 0; i < size; i += PAGE_SIZE)
		vmm_unmap_page(addr + i);
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
	*dir_ptr = mkentry(g_pmm_addr, DIR_FLAG_P | DIR_FLAG_RW);
	uint32_t *tbl_ptr = TBL_VADDR(dir_id);
	uint32_t tbl_id = TBL_ID(g_pmm_bitmap);
	for (uint32_t i = 0; i < bitmap_pages; ++i)
		tbl_ptr[tbl_id + i] = mkentry(g_pmm_addr + PAGE_SIZE + i * PAGE_SIZE, TBL_FLAG_P | TBL_FLAG_RW);

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
		assert(!*dir, "non-NULL heap page table 0x%lx\n", addr);
		*dir = mkentry(pmm_alloc_page(), DIR_FLAG_P | DIR_FLAG_RW);
		uint32_t *tbl_ptr = TBL_VADDR(dir_id);
		memset(tbl_ptr, 0, PAGE_SIZE);
	}
}

void paging_init(uint32_t addr, uint32_t size)
{
	g_pmm_addr = addr;
	g_pmm_size = size;
	printf("initializing memory 0x%lx-0x%lx (%lu)\n", addr, addr + size, size);
	init_physical_maps();
	init_heap_pages_tables();
	g_vmm_heap.addr = VADDR_HEAP_BEG;
	g_vmm_heap.size = VADDR_HEAP_END - VADDR_HEAP_BEG;
	TAILQ_INIT(&g_vmm_heap.ranges);
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
	printf("kernel heap:\n");
	struct vmm_range *item;
	TAILQ_FOREACH(item, &g_vmm_heap.ranges, chain)
		printf("0x%lx - 0x%lx: 0x%lx bytes\n", item->addr, item->addr + item->size, item->size);
}

void paging_dumpinfo()
{
	pmm_dumpinfo();
	vmm_dumpinfo();
}
