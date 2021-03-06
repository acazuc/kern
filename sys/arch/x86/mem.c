#include "x86.h"
#include "asm.h"

#include <sys/proc.h>
#include <sys/pcpu.h>
#include <inttypes.h>
#include <sys/vmm.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
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
 * IMPORTANT: assumption is made that the all the directory
 * pages for all the process will always be mapped in the
 * kernel heap region
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

#define PAGE_MASK 0xFFF

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
	assert(addr >= g_pmm_addr, "free_page of invalid address (too low): %" PRIx32 "\n", addr);
	assert(addr < g_pmm_addr + g_pmm_size, "free_page of invalid address (too high): %" PRIx32 "\n", addr);
	uint32_t delta = (addr - g_pmm_addr) / PAGE_SIZE;
	uint32_t *bitmap = &g_pmm_bitmap[delta / 32];
	uint32_t mask = (1 << (delta % 32));
	assert(*bitmap & mask, "free_page of unallocated page: %" PRIx32 "\n", addr);
	*bitmap &= ~mask;
	if (delta < g_pmm_bitmap_first)
		g_pmm_bitmap_first = delta;
}

void paging_alloc(uint32_t addr)
{
	addr &= ~PAGE_MASK;
	struct vmm_ctx *ctx = curthread ? curthread->proc->vmm_ctx : NULL;
	uint32_t dir_id = DIR_ID(addr);
	uint32_t dir = ctx ? ctx->dir[dir_id] : DIR_VADDR[dir_id];
	assert(dir & DIR_FLAG_P, "paging non-allocated vmem dir 0x%" PRIx32 "\n", addr);
	uint32_t *tbl_ptr = vmap(dir & ~PAGE_MASK, PAGE_SIZE);
	assert(tbl_ptr, "can't map tbl\n");
	uint32_t tbl_id = TBL_ID(addr);
	uint32_t *tbl = &tbl_ptr[tbl_id];
	assert(!(*tbl & TBL_FLAG_P), "allocating already present page 0x%" PRIx32 "\n", addr);
	assert(*tbl & TBL_FLAG_V, "paging non-allocated vmem tbl 0x%" PRIx32 "\n", addr);
	uint32_t f = TBL_FLAG_RW | TBL_FLAG_P;
	if (*tbl & TBL_FLAG_US)
		f |= TBL_FLAG_US;
	uint32_t paddr = pmm_alloc_page();
	*tbl = mkentry(paddr, f);
	/*if (f & TBL_FLAG_US)
	{
		void *dst = vmap(paddr, PAGE_SIZE);
		assert(dst, "can't map created page\n");
		memset(dst, 0, PAGE_SIZE);
		vunmap(dst, PAGE_SIZE);
	}*/
	vunmap(tbl_ptr, PAGE_SIZE);
}

static void vmm_alloc_page(struct vmm_ctx *ctx, uint32_t addr, int user)
{
	assert(!(addr & PAGE_MASK), "alloc unaligned page 0x%" PRIx32 "\n", addr);
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
	assert(!(*tbl & TBL_FLAG_P), "vmalloc already created page 0x%" PRIx32 ", 0x%" PRIx32 "\n", addr, *tbl);
	assert(!(*tbl & TBL_FLAG_V), "vmalloc already allocated page 0x%" PRIx32 ", 0x%" PRIx32 "\n", addr, *tbl);
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
	assert(!(addr & PAGE_MASK), "vmap unaligned page 0x%" PRIx32 "\n", addr);
	assert(!(paddr & PAGE_MASK), "vmap unaligned physical page 0x%" PRIx32 "\n", addr);
	uint32_t dir_id = DIR_ID(addr);
	uint32_t *dir = &DIR_VADDR[dir_id];
	if (!(*dir & DIR_FLAG_P))
	{
		*dir = mkentry(pmm_alloc_page(), DIR_FLAG_RW | DIR_FLAG_P);
		memset(TBL_VADDR(dir_id), 0, PAGE_SIZE);
	}
	uint32_t *tbl_ptr = TBL_VADDR(dir_id);
	uint32_t tbl_id = TBL_ID(addr);
	uint32_t *tbl = &tbl_ptr[tbl_id];
	assert(!(*tbl & TBL_FLAG_P), "vmap already created page 0x%" PRIx32 "\n", addr);
	assert(!(*tbl & TBL_FLAG_V), "vmap already allocated page 0x%" PRIx32 "\n", addr);
	*tbl = mkentry(paddr, TBL_FLAG_RW | TBL_FLAG_P);
	invlpg(addr);
}

static void vmm_map_user_page(struct vmm_ctx *ctx, uint32_t addr, uint32_t uaddr)
{
	assert(!(addr & PAGE_MASK), "vmap unaligned page 0x%" PRIx32 "\n", addr);
	assert(!(uaddr & PAGE_MASK), "vmap unaligned user page 0x%" PRIx32 "\n", uaddr);
	uint32_t dir_id = DIR_ID(uaddr);
	uint32_t *dir = &ctx->dir[dir_id];
	uint32_t *tbl_ptr;
	if (!(*dir & DIR_FLAG_P))
	{
		uint32_t paddr = pmm_alloc_page();
		*dir = mkentry(paddr, DIR_FLAG_RW | DIR_FLAG_P | DIR_FLAG_US);
		tbl_ptr = vmap(paddr, PAGE_SIZE);
		assert(tbl_ptr, "can't vmap tbl\n");
		memset(tbl_ptr, 0, PAGE_SIZE);
	}
	else
	{
		tbl_ptr = vmap(*dir & ~PAGE_MASK, PAGE_SIZE);
		assert(tbl_ptr, "can't vmap tbl\n");
	}
	uint32_t tbl_id = TBL_ID(uaddr);
	uint32_t *tbl = &tbl_ptr[tbl_id];
	uint32_t paddr;
	if (*tbl & TBL_FLAG_P)
	{
		paddr = *tbl & ~PAGE_MASK;
	}
	else
	{
		assert((*tbl & TBL_FLAG_V), "vmap non allocated page 0x%" PRIx32 "\n", uaddr);
		paddr = pmm_alloc_page();
		*tbl = mkentry(paddr, TBL_FLAG_RW | TBL_FLAG_P | TBL_FLAG_US);
	}
	vunmap(tbl_ptr, PAGE_SIZE);
	vmm_map_page(addr, paddr);
}

static void vmm_unmap_page(uint32_t addr)
{
	uint32_t dir_id = DIR_ID(addr);
	uint32_t dir = DIR_VADDR[dir_id];
	assert(dir & DIR_FLAG_P, "free of unexisting dir\n");
	uint32_t *tbl_ptr = TBL_VADDR(dir_id);
	uint32_t tbl_id = TBL_ID(addr);
	uint32_t tbl = tbl_ptr[tbl_id];
	assert(tbl & TBL_FLAG_P, "unmap non-mapped page 0x%08" PRIx32 "\n", addr);
	tbl_ptr[tbl_id] = mkentry(0, 0);
	invlpg(addr);
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

struct vmm_ctx *vmm_ctx_create(void)
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

void vmm_ctx_delete(struct vmm_ctx *ctx)
{
	for (size_t i = 0; i < 768; ++i)
	{
		if (!(ctx->dir[i] & DIR_FLAG_P))
			continue;
		uint32_t *tbl_ptr = vmap(ctx->dir[i] & ~PAGE_MASK, PAGE_SIZE);
		assert(tbl_ptr, "can't map tbl ptr\n");
		for (size_t j = 0; j < 1024; ++j)
		{
			if (!(tbl_ptr[j] & TBL_FLAG_P))
				continue;
			pmm_free_page(tbl_ptr[j] & ~PAGE_MASK);
		}
		vunmap(tbl_ptr, PAGE_SIZE);
		pmm_free_page(ctx->dir[i] & ~PAGE_MASK);
	}
	size_t size = sizeof(*ctx) + PAGE_SIZE - 1;
	size -= size % PAGE_SIZE;
	vunmap(ctx, size);
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
	assert(!(addr & PAGE_MASK), "vmalloc unaligned addr 0x%" PRIx32 "\n", addr);
	assert(!(size & PAGE_MASK), "vmalloc unaligned size 0x%" PRIx32 "\n", size);
	if (vmm_get_free_range(ctx ? &ctx->region : &g_vmm_heap, addr, size, &addr))
		return NULL;
	assert(!(addr & PAGE_MASK), "vmalloc unaligned data 0x%" PRIx32 "\n", addr);
	for (uint32_t i = 0; i < size; i += PAGE_SIZE)
		vmm_alloc_page(ctx, addr + i, user);
	return (void*)addr;
}

static void vfree_zone(struct vmm_ctx *ctx, void *ptr, size_t size)
{
	uint32_t addr = (uint32_t)ptr;
	assert(!(addr & PAGE_MASK), "free of unaligned addr: 0x%" PRIx32 "\n", addr);
	assert(!(size & PAGE_MASK), "free of unaligned size: 0x%" PRIx32 "\n", size);
	for (uint32_t i = 0; i < size; i += PAGE_SIZE)
		vmm_free_page(ctx, addr + i);
	vmm_set_free_range(ctx ? &ctx->region : &g_vmm_heap, addr, size);
}

void *vmalloc(size_t size)
{
	/*
	 * NB: dst MUST be NULL
	 * if not, allocation could occur during malloc call(), doing recursive
	 * allocation, stucking the system
	 */
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
	assert(!(paddr & PAGE_MASK), "vmap unaligned addr 0x%" PRIx32 "\n", paddr);
	assert(!(size  & PAGE_MASK), "vmap unaligned size 0x%" PRIx32 "\n", size);
	uint32_t addr;
	if (vmm_get_free_range(&g_vmm_heap, 0, size, &addr))
		return NULL;
	assert(!(addr & PAGE_MASK), "vmap unaligned data 0x%" PRIx32 "\n", addr);
	for (uint32_t i = 0; i < size; i += PAGE_SIZE)
		vmm_map_page(addr + i, paddr + i);
	return (void*)addr;
}

void *vmap_user(struct vmm_ctx *ctx, void *ptr, size_t size)
{
	assert(ctx, "no vmm ctx given\n");
	uint32_t uaddr = (uint32_t)ptr;
	assert(!(uaddr & PAGE_MASK), "vmap unaligned addr 0x%" PRIx32 "\n", uaddr);
	assert(!(size & PAGE_MASK), "vmap unaligned size 0x%" PRIx32 "\n", size);
	uint32_t addr;
	if (vmm_get_free_range(&g_vmm_heap, 0, size, &addr))
		return NULL;
	assert(!(addr & PAGE_MASK), "vmap unaligned data 0x%" PRIx32 "\n", addr);
	for (uint32_t i = 0; i < size; i += PAGE_SIZE)
		vmm_map_user_page(ctx, addr + i, uaddr + i);
	return (void*)addr;
}

void vunmap(void *ptr, size_t size)
{
	uint32_t addr = (uint32_t)ptr;
	assert(!(addr & PAGE_MASK), "vunmap unaligned addr 0x%" PRIx32 "\n", addr);
	assert(!(size & PAGE_MASK), "vunmap unaligned size 0x%" PRIx32 "\n", size);
	for (uint32_t i = 0; i < size; i += PAGE_SIZE)
		vmm_unmap_page(addr + i);
	vmm_set_free_range(&g_vmm_heap, addr, size);
}

static void init_physical_maps(void)
{
	uint32_t pages_count = g_pmm_size / PAGE_SIZE;
	g_pmm_bitmap_size = (pages_count + 31) / 32;
	if (g_pmm_bitmap_size * 32 >= PAGE_SIZE * 0x400) /* arbitrary: must not be greater than a single dir */
		panic("bitmap size too long: %" PRId32 " / %d", g_pmm_bitmap_size * 4, PAGE_SIZE * 0x400);

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
		assert(!*dir, "non-NULL heap page table 0x%" PRIx32 "\n", addr);
		*dir = mkentry(pmm_alloc_page(), DIR_FLAG_P | DIR_FLAG_RW);
		uint32_t *tbl_ptr = TBL_VADDR(dir_id);
		memset(tbl_ptr, 0, PAGE_SIZE);
	}
}

void paging_init(uint32_t addr, uint32_t size)
{
	g_pmm_addr = addr;
	g_pmm_size = size;
	printf("initializing memory 0x%" PRIx32 "-0x%" PRIx32 " (%" PRIu32 ")\n", addr, addr + size, size);
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
	printf("pages used: 0x%05" PRIx32 " / 0x%05" PRIx32 " (%d%%); first available: %05" PRIx32 "\n", pages_used, g_pmm_bitmap_size * 32, (int)(pages_used / (float)(g_pmm_bitmap_size * 32) * 100), g_pmm_bitmap_first);
}

static void vmm_dumpinfo(void)
{
	printf("kernel heap:\n");
	struct vmm_range *item;
	TAILQ_FOREACH(item, &g_vmm_heap.ranges, chain)
		printf("0x%" PRIx32 " - 0x%" PRIx32 ": 0x%" PRIx32 " bytes\n", item->addr, item->addr + item->size, item->size);
}

void paging_dumpinfo()
{
	pmm_dumpinfo();
	vmm_dumpinfo();
}
