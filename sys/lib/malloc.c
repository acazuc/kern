#include "arch/arch.h"

#include <sys/queue.h>
#include <sys/std.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* XXX
 * allocation should be split between metadata and data
 * because allocations must be size-aligned (i.e: 4k alloc should be 4k aligned)
 * it cause a lot of unused memory because of padding between struct page and data
 * (almost 4k for a 4k allocation)
 *
 * maybe the metadata should be allocated in another zone than the data, allowing something
 * like multiple struct page inside a metadata page allocation
 */

#define BLOCKS_SIZE ((BLOCKS_COUNT + BLOCK_BITS - 1) / BLOCK_BITS)
#define BLOCKS_COUNT 128 /* XXX: per-block type size */
#define BLOCK_BITS (sizeof(size_t) * 8)

#define MALLOC_LOCK()
#define MALLOC_UNLOCK()

enum block_type
{
	BLOCK_1,
	BLOCK_2,
	BLOCK_4,
	BLOCK_8,
	BLOCK_16,
	BLOCK_32,
	BLOCK_64,
	BLOCK_128,
	BLOCK_256,
	BLOCK_512,
	BLOCK_1024,
	BLOCK_2048,
	BLOCK_LARGE
};

static const char *g_block_str[] =
{
	"1",
	"2",
	"4",
	"8",
	"16",
	"32",
	"64",
	"128",
	"256",
	"512",
	"1024",
	"2048",
	"LARGE"
};

static const size_t g_block_sizes[] =
{
	1,
	2,
	4,
	8,
	16,
	32,
	64,
	128,
	256,
	512,
	1024,
	2048,
	0
};

struct page
{
	enum block_type type;
	size_t page_size; /* size of vmalloc */
	size_t data_size; /* size of payload (for large) */
	uint8_t *addr;
	TAILQ_ENTRY(page) chain;
	size_t blocks[];
};

static TAILQ_HEAD(, page) g_pages[BLOCK_LARGE + 1];

static enum block_type get_block_type(size_t size)
{
	for (size_t i = 0; i < sizeof(g_block_sizes) / sizeof(*g_block_sizes); ++i)
	{
		if (size <= g_block_sizes[i])
			return i;
	}
	return BLOCK_LARGE;
}

static struct page *alloc_page(enum block_type type, size_t size)
{
	struct page *page;
	size_t alloc_size;
	size_t blocks_size;
	size_t meta_size;
	size_t align_size;

	if (type != BLOCK_LARGE)
	{
		blocks_size = sizeof(size_t) * BLOCKS_SIZE;
		align_size = g_block_sizes[type];
	}
	else
	{
		blocks_size = 0;
		align_size = PAGE_SIZE;
	}
	meta_size = sizeof(*page) + blocks_size;
	meta_size += align_size - 1; /* align blocks */
	meta_size -= meta_size % align_size;
	alloc_size = size + meta_size;
	alloc_size += PAGE_SIZE - 1;
	alloc_size -= alloc_size % PAGE_SIZE;
	page = vmalloc(alloc_size);
	if (!page)
		return NULL;
	page->type = type;
	page->page_size = alloc_size;
	page->data_size = size;
	page->addr = (uint8_t*)page + meta_size;
	memset(page->blocks, 0, blocks_size);
	return page;
}

static int is_page_free(struct page *page)
{
	for (size_t i = 0; i < BLOCKS_SIZE; ++i)
	{
		if (page->blocks[i])
			return 0;
	}
	return 1;
}

static void check_free_pages(enum block_type type)
{
	int one_free = 0;
	struct page *lst;
	struct page *nxt;
	TAILQ_FOREACH_SAFE(lst, &g_pages[type], chain, nxt)
	{
		if (lst->type != type || !is_page_free(lst))
			continue;
		if (!one_free)
		{
			one_free = 1;
			continue;
		}
		TAILQ_REMOVE(&g_pages[type], lst, chain);
		vfree(lst, lst->page_size);
	}
}

static void *create_new_block(enum block_type type, size_t size)
{
	struct page *page;

	page = alloc_page(type, type == BLOCK_LARGE ? size : g_block_sizes[type] * BLOCKS_COUNT);
	if (!page)
		return NULL;
	page->blocks[0] = 1;
	TAILQ_INSERT_TAIL(&g_pages[type], page, chain);
	return page->addr;
}

static int get_first_free(struct page *page)
{
	for (size_t i = 0; i < BLOCKS_SIZE; ++i)
	{
		if (page->blocks[i] == (size_t)-1)
			continue;
		size_t block = page->blocks[i];
		for (size_t j = 0; j < BLOCK_BITS; ++j)
		{
			if (!(block & (1 << j)))
				return i * BLOCK_BITS + j;
		}
		panic("block != -1 but no bit found");
	}
	return -1;
}

static void *get_existing_block(enum block_type type)
{
	struct page *lst;
	TAILQ_FOREACH(lst, &g_pages[type], chain)
	{
		if (lst->type != type)
			continue;
		int i = get_first_free(lst);
		if (i == -1)
			continue;
		lst->blocks[i / BLOCK_BITS] |= (1 << i % BLOCK_BITS);
		return lst->addr + i * g_block_sizes[type];
	}
	return NULL;
}

static void *realloc_large(struct page *lst, void *ptr, size_t size, uint32_t flags)
{
	void *addr;
	size_t len;

	(void)flags;
	addr = create_new_block(BLOCK_LARGE, size);
	if (!addr)
	{
		MALLOC_UNLOCK();
		return NULL;
	}
	len = lst->data_size;
	if (size < len)
		len = size;
	memcpy(addr, ptr, len);
	TAILQ_REMOVE(&g_pages[lst->type], lst, chain);
	vfree(lst, lst->page_size);
	MALLOC_UNLOCK();
	return addr;
}

static void *realloc_blocky(struct page *lst, void *ptr, size_t size, uint32_t flags)
{
	if (size <= g_block_sizes[lst->type])
	{
		MALLOC_UNLOCK();
		return ptr;
	}
	MALLOC_UNLOCK();
	void *addr = malloc(size, flags);
	if (!addr)
		return NULL;
	memcpy(addr, ptr, g_block_sizes[lst->type]);
	free(ptr);
	return addr;
}

void *malloc(size_t size, uint32_t flags)
{
	enum block_type type;
	void *addr;

	(void)flags;
	if (!size)
		return NULL;
	type = get_block_type(size);
	MALLOC_LOCK();
	if (type == BLOCK_LARGE || !(addr = get_existing_block(type)))
	{
		addr = create_new_block(type, size);
		if (!addr)
		{
			MALLOC_UNLOCK();
			return NULL;
		}
	}
	MALLOC_UNLOCK();
	if (flags & M_ZERO)
		memset(addr, 0, size);
	return addr;
}

void free(void *ptr)
{
	if (!ptr)
		return;
	MALLOC_LOCK();
	struct page *lst;
	for (size_t i = 0; i < sizeof(g_pages) / sizeof(*g_pages); ++i)
	{
		TAILQ_FOREACH(lst, &g_pages[i], chain)
		{
			if (lst->type == BLOCK_LARGE)
			{
				if (ptr != lst->addr)
					continue;
				TAILQ_REMOVE(&g_pages[i], lst, chain);
				vfree(lst, lst->page_size);
				MALLOC_UNLOCK();
				return;
			}
			if ((uint8_t*)ptr < lst->addr
			 || (uint8_t*)ptr >= lst->addr + g_block_sizes[lst->type] * BLOCKS_COUNT)
				continue;
			size_t item = ((uint8_t*)ptr - lst->addr) / g_block_sizes[lst->type];
			if (lst->addr + g_block_sizes[lst->type] * item != (uint8_t*)ptr)
				continue;
			size_t mask = (1 << (item % BLOCK_BITS));
			size_t *block = &lst->blocks[item / BLOCK_BITS];
			if (!(*block & mask))
				panic("double free %p\n", ptr);
			*block &= ~mask;
			check_free_pages(lst->type);
			MALLOC_UNLOCK();
			return;
		}
	}
	panic("free unknown addr: %p\n", ptr);
	MALLOC_UNLOCK();
}

void *realloc(void *ptr, size_t size, uint32_t flags)
{
	(void)flags;
	if (!ptr)
		return malloc(size, flags);
	if (!size)
	{
		free(ptr);
		return NULL;
	}
	MALLOC_LOCK();
	struct page *lst;
	for (size_t i = 0; i < sizeof(g_pages) / sizeof(*g_pages); ++i)
	{
		TAILQ_FOREACH(lst, &g_pages[i], chain)
		{
			if (lst->type == BLOCK_LARGE)
			{
				if ((uint8_t*)ptr != lst->addr)
					continue;
				return realloc_large(lst, ptr, size, flags);
			}
			if ((uint8_t*)ptr < lst->addr
			 || (uint8_t*)ptr >= lst->addr + g_block_sizes[lst->type] * BLOCKS_COUNT)
				continue;
			size_t item = ((uint8_t*)ptr - lst->addr) / g_block_sizes[lst->type];
			if (lst->addr + g_block_sizes[lst->type] * item == ptr)
				return realloc_blocky(lst, ptr, size, flags);
			MALLOC_UNLOCK();
			return NULL;
		}
	}
	MALLOC_UNLOCK();
	return NULL;
}

static void print_block(size_t start, size_t end, size_t len)
{
	printf(" 0x%lx - 0x%lx : 0x%lx bytes\n", start, end, len);
}

static void c_e(void **start, void *end, size_t *total)
{
	*total += end - *start;
	print_block((size_t)*start, (size_t)end, (size_t)(end - *start));
	*start = NULL;
}

static void print_page(struct page *page, size_t *total)
{
	void *start;
	int i;

	if (page->type == BLOCK_LARGE)
	{
		*total += page->data_size;
		print_block((size_t)page->addr, (size_t)(page->addr + page->data_size), page->data_size);
		return;
	}
	start = NULL;
	for (i = 0; i < BLOCKS_COUNT; ++i)
	{
		size_t set = page->blocks[i / BLOCK_BITS] & (1 << (i % BLOCK_BITS));
		if (set && !start)
			start = page->addr + i * g_block_sizes[page->type];
		else if (!set && start)
			c_e(&start, page->addr + i * g_block_sizes[page->type], total);
	}
	if (start)
		c_e(&start, page->addr + i * g_block_sizes[page->type], total);
}

static struct page *try_push(size_t min)
{
	struct page *lowest;
	struct page *lst;
	size_t lowest_val;

	lowest = NULL;
	lowest_val = -1;
	for (size_t i = 0; i < sizeof(g_pages) / sizeof(*g_pages); ++i)
	{
		TAILQ_FOREACH(lst, &g_pages[i], chain)
		{
			if ((lowest_val == 0 || (size_t)lst < lowest_val) && (size_t)lst > min)
			{
				lowest = lst;
				lowest_val = (size_t)lowest;
			}
		}
	}
	return lowest;
}

void alloc_init(void)
{
	for (size_t i = 0; i < sizeof(g_pages) / sizeof(*g_pages); ++i)
		TAILQ_INIT(&g_pages[i]);
}

void show_alloc_mem(void)
{
	struct page *tmp;
	size_t total;

	MALLOC_LOCK();
	total = 0;
	tmp = NULL;
	while (1)
	{
		if (!(tmp = try_push((size_t)tmp)))
			break;
		printf("%s : %p (0x%lx)\n", g_block_str[tmp->type], tmp, tmp->page_size);
		print_page(tmp, &total);
	}
	printf("total: %lu bytes\n", total);
	MALLOC_UNLOCK();
}
