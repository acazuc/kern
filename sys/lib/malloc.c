#include "arch/arch.h"

#include <sys/std.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define BLOCKS_SIZE ((BLOCKS_COUNT + 31) / 32)
#define BLOCKS_COUNT 128 /* XXX: per-block type size */

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
	BLOCK_4096,
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
	"4096",
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
	4096,
	0
};

struct page
{
	enum block_type type;
	size_t size;
	uint8_t *addr;
	struct page *next;
	uint32_t blocks[];
};

static struct page *g_pages;

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

	if (type != BLOCK_LARGE)
		blocks_size = sizeof(uint32_t) * BLOCKS_SIZE;
	else
		blocks_size = 0;
	alloc_size = size + sizeof(*page) + blocks_size + PAGE_SIZE - 1;
	alloc_size -= alloc_size % PAGE_SIZE;
	/* XXX: create alloc_size var in page, set size = alloc_size - sizeof(*page) */
	page = vmalloc(alloc_size);
	if (!page)
		return NULL;
	page->type = type;
	page->size = alloc_size;
	page->addr = (uint8_t*)page + sizeof(*page) + blocks_size;
	page->next = NULL;
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

static void remove_page(struct page *page)
{
	struct page *prv;
	struct page *lst;

	prv = NULL;
	lst = g_pages;
	while (lst)
	{
		if (lst == page)
		{
			if (!prv)
				g_pages = lst->next;
			else
				prv->next = lst->next;
			return ;
		}
		prv = lst;
		lst = lst->next;
	}
}

static void check_free_pages(enum block_type type)
{
	int one_free = 0;
	for (struct page *lst = g_pages; lst;)
	{
		if (lst->type != type || !is_page_free(lst))
		{
			lst = lst->next;
			continue;
		}
		if (!one_free)
		{
			one_free = 1;
			lst = lst->next;
			continue;
		}
		struct page *nxt = lst->next;
		remove_page(lst);
		vfree(lst, lst->size);
		lst = nxt;
	}
}

static void push_new_page(struct page *new)
{
	if (!g_pages)
	{
		g_pages = new;
		return;
	}

	struct page *lst = g_pages;
	while (lst->next)
		lst = lst->next;
	lst->next = new;
}

static void *create_new_block(enum block_type type, size_t size)
{
	struct page *page;

	page = alloc_page(type, type == BLOCK_LARGE ? size : g_block_sizes[type] * BLOCKS_COUNT);
	if (!page)
		return NULL;
	page->blocks[0] = 1;
	push_new_page(page);
	return page->addr;
}

static int get_first_free(struct page *page)
{
	for (size_t i = 0; i < BLOCKS_SIZE; ++i)
	{
		if (page->blocks[i] == 0xFFFFFFFF)
			continue;
		uint32_t block = page->blocks[i];
		for (uint32_t j = 0; j < 32; ++j)
		{
			if (!(block & (1 << j)))
				return i * 32 + j;
		}
		panic("block != 0xFFFFFFFF but no bit found");
	}
	return -1;
}

static void *get_existing_block(enum block_type type)
{
	for (struct page *lst = g_pages; lst; lst = lst->next)
	{
		int i;
		if (lst->type == type && (i = get_first_free(lst)) != -1)
		{
			lst->blocks[i / 32] |= (1 << i % 32);
			return lst->addr + i * g_block_sizes[type];
		}
	}
	return NULL;
}

static void *realloc_large(struct page *lst, void *ptr, size_t size, uint32_t flags)
{
	void *addr;
	size_t len;

	addr = create_new_block(BLOCK_LARGE, size);
	if (!addr)
	{
		MALLOC_UNLOCK();
		return NULL;
	}
	len = lst->size - sizeof(*lst);
	if (size < len)
		len = size;
	memcpy(addr, ptr, len);
	remove_page(lst);
	vfree(lst, lst->size);
	MALLOC_UNLOCK();
	if (flags & M_ZERO)
		memset(addr, 0, size);
	return addr;
}

static void *realloc_blocky(struct page *lst, void *ptr, size_t size, uint32_t flags)
{
	void *addr;
	size_t len;

	if (size <= g_block_sizes[lst->type])
	{
		MALLOC_UNLOCK();
		return ptr;
	}
	addr = create_new_block(get_block_type(size), size);
	if (!addr)
	{
		MALLOC_UNLOCK();
		return NULL;
	}
	len = lst->size - sizeof(*lst);
	if (size < len)
		len = size;
	memcpy(addr, ptr, len);
	free(ptr);
	MALLOC_UNLOCK();
	if (flags & M_ZERO)
		memset(addr, 0, size);
	return addr;
}

void *malloc(size_t size, uint32_t flags)
{
	enum block_type type;
	void *addr;

	(void)flags;
	MALLOC_LOCK();
	if (!size)
		return NULL;
	type = get_block_type(size);
	if (type == BLOCK_LARGE || !(addr = get_existing_block(type)))
	{
		addr = create_new_block(type, size);
		if (!addr)
			return NULL;
	}
	if (flags & M_ZERO)
		memset(addr, 0, size);
	return addr;
}

void free(void *ptr)
{
	struct page *prv;

	if (!ptr)
		return;
	MALLOC_LOCK();
	prv = NULL;
	for (struct page *lst = g_pages; lst; prv = lst, lst = lst->next)
	{
		if (lst->type == BLOCK_LARGE)
		{
			if (ptr != lst->addr)
				continue;
			if (prv)
				prv->next = lst->next;
			else
				g_pages = lst->next;
			vfree(lst, lst->size);
			MALLOC_UNLOCK();
			return;
		}
		if ((uint8_t*)ptr < lst->addr
		 || (uint8_t*)ptr > lst->addr + g_block_sizes[lst->type] * BLOCKS_COUNT)
			continue;
		size_t item = ((uint8_t*)ptr - lst->addr) / g_block_sizes[lst->type];
		if (lst->addr + g_block_sizes[lst->type] * item != (uint8_t*)ptr)
			continue;
		uint32_t mask = (1 << (item % 32));
		uint32_t *block = &lst->blocks[item / 32];
		if (!(*block & mask))
			panic("double free %p\n", ptr);
		*block &= ~mask;
		check_free_pages(lst->type);
		return;
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
	for (struct page *lst = g_pages; lst; lst = lst->next)
	{
		if (lst->type == BLOCK_LARGE)
		{
			if ((uint8_t*)ptr == lst->addr)
				return realloc_large(lst, ptr, size, flags);
			continue;
		}
		if ((uint8_t*)ptr >= lst->addr
		 && (uint8_t*)ptr <= lst->addr + g_block_sizes[lst->type] * BLOCKS_COUNT)
		{
			uint32_t item = ((uint8_t*)ptr - lst->addr) / g_block_sizes[lst->type];
			if (lst->addr + g_block_sizes[lst->type] * item == ptr)
				return realloc_blocky(lst, ptr, size, flags);
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
		*total += page->size - sizeof(struct page);
		print_block((size_t)page->addr, (size_t)(page->addr + page->size), page->size);
		return;
	}
	start = NULL;
	for (i = 0; i < BLOCKS_COUNT; ++i)
	{
		uint32_t set = page->blocks[i / 32] & (1 << (i % 32));
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
	size_t lowest_val;

	lowest = NULL;
	lowest_val = -1;
	for (struct page *lst = g_pages; lst; lst = lst->next)
	{
		if ((lowest_val == 0 || (size_t)lst < lowest_val) && (size_t)lst > min)
		{
			lowest = lst;
			lowest_val = (size_t)lowest;
		}
	}
	return lowest;
}

void show_alloc_mem()
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
		printf("%s : %p (0x%lx)\n", g_block_str[tmp->type], tmp, tmp->size);
		print_page(tmp, &total);
	}
	printf("total: %lu bytes\n", total);
	MALLOC_UNLOCK();
}
