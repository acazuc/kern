#include "std.h"

#include "arch/arch.h"

#include <stdbool.h>

#define BLOCKS_SIZE ((PAGE_SIZE + 31) / 32)
#define PAGE_SIZE 1024

#define MALLOC_LOCK()
#define MALLOC_UNLOCK()

typedef struct page page_t;

enum block_type
{
	BLOCK_TINY,
	BLOCK_SMALL,
	BLOCK_LARGE,
};

static const char *g_block_str[] =
{
	"TINY",
	"SMALL",
	"LARGE"
};

static const uint32_t g_block_sizes[] =
{
	128,
	1024,
	0
};

static const uint32_t g_page_sizes[] =
{
	128 * PAGE_SIZE,
	1024 * PAGE_SIZE,
	0
};

struct page
{
	enum block_type type;
	size_t size;
	uint8_t *addr;
	uint32_t blocks[BLOCKS_SIZE];
	page_t *next;
};

static page_t *g_pages;

static enum block_type get_block_type(size_t size)
{
	if (size <= g_block_sizes[BLOCK_TINY])
		return BLOCK_TINY;
	if (size <= g_block_sizes[BLOCK_SMALL])
		return BLOCK_SMALL;
	return BLOCK_LARGE;
}

static page_t *alloc_page(enum block_type type, size_t size)
{
	page_t *page;
	size_t alloc_size;

	alloc_size = (size + sizeof(*page) + 4095) & ~(0xFFF);
	page = vmalloc(alloc_size);
	if (!page)
		return NULL;
	page->type = type;
	page->size = alloc_size;
	page->addr = (uint8_t*)page + sizeof(*page);
	page->next = NULL;
	if (type == BLOCK_TINY || type == BLOCK_SMALL)
		memset(page->blocks, 0, sizeof(page->blocks));
	return page;
}

static bool is_page_free(page_t *page)
{
	for (size_t i = 0; i < BLOCKS_SIZE; ++i)
	{
		if (page->blocks[i])
			return false;
	}
	return true;
}

static void remove_page(page_t *page)
{
	page_t *prv;
	page_t *lst;

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
	bool one_free = false;
	for (page_t *lst = g_pages; lst;)
	{
		if (lst->type != type || !is_page_free(lst))
		{
			lst = lst->next;
			continue;
		}
		if (!one_free)
		{
			one_free = true;
			lst = lst->next;
			continue;
		}
		page_t *nxt = lst->next;
		remove_page(lst);
		vfree(lst, lst->size);
		lst = nxt;
	}
}

static void push_new_page(page_t *new)
{
	if (!g_pages)
	{
		g_pages = new;
		return;
	}

	page_t *lst = g_pages;
	while (lst->next)
		lst = lst->next;
	lst->next = new;
}

static void *create_new_block(enum block_type type, size_t size)
{
	page_t *page;

	page = alloc_page(type, type == BLOCK_LARGE ? size : g_page_sizes[type]);
	if (!page)
		return NULL;
	page->blocks[0] = 1;
	push_new_page(page);
	return page->addr;
}

static int get_first_free(page_t *page)
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
	for (page_t *lst = g_pages; lst; lst = lst->next)
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

static void *realloc_large(page_t *lst, void *ptr, size_t size)
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
	return addr;
}

static void *realloc_small_tiny(page_t *lst, void *ptr, size_t size)
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
	return addr;
}

void *malloc(size_t size)
{
	enum block_type type;
	void *addr;

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
	return addr;
}

void free(void *ptr)
{
	page_t *prv;

	if (!ptr)
		return;
	MALLOC_LOCK();
	prv = NULL;
	for (page_t *lst = g_pages; lst; prv = lst, lst = lst->next)
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
		 || (uint8_t*)ptr > lst->addr + g_page_sizes[lst->type])
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

void *realloc(void *ptr, size_t size)
{
	if (!ptr)
		return malloc(size);
	if (!size)
	{
		free(ptr);
		return NULL;
	}
	MALLOC_LOCK();
	for (page_t *lst = g_pages; lst; lst = lst->next)
	{
		if (lst->type == BLOCK_LARGE)
		{
			if ((uint8_t*)ptr == lst->addr)
				return realloc_large(lst, ptr, size);
			continue;
		}
		if ((uint8_t*)ptr >= lst->addr
		 && (uint8_t*)ptr <= lst->addr + g_page_sizes[lst->type])
		{
			uint32_t item = ((uint8_t*)ptr - lst->addr) / g_block_sizes[lst->type];
			if (lst->addr + g_block_sizes[lst->type] * item == ptr)
				return realloc_small_tiny(lst, ptr, size);
			return NULL;
		}
	}
	MALLOC_UNLOCK();
	return NULL;
}

void *zalloc(size_t size)
{
	char *addr = malloc(size);
	if (addr)
		memset(addr, 0, size);
	return addr;
}

static void print_block(size_t start, size_t end, size_t len)
{
	printf("0x%08lx - 0x%08lx : 0x%lx bytes\n", start, end, len);
}

static void c_e(void **start, void *end, size_t *total)
{
	*total += end - *start;
	print_block((size_t)*start, (size_t)end, (size_t)(end - *start));
	*start = NULL;
}

static void print_page(page_t *page, size_t *total)
{
	void *start;
	int i;

	if (page->type == BLOCK_LARGE)
	{
		*total += page->size - sizeof(page_t);
		print_block((size_t)page->addr, (size_t)(page->addr + page->size), page->size);
		return;
	}
	start = NULL;
	for (i = 0; i < PAGE_SIZE; ++i)
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

static page_t *try_push(size_t min)
{
	page_t *lowest;
	size_t lowest_val;

	lowest = NULL;
	lowest_val = -1;
	for (page_t *lst = g_pages; lst; lst = lst->next)
	{
		if ((lowest_val == 0 || (size_t)lst < lowest_val) && (size_t)lst > min)
		{
			lowest = lst;
			lowest_val = (size_t)lowest;
		}
	}
	return lowest;
}

void show_alloc_mem(void)
{
	page_t *tmp;
	size_t total;

	MALLOC_LOCK();
	total = 0;
	tmp = NULL;
	while (1)
	{
		if (!(tmp = try_push((size_t)tmp)))
			break;
		printf("%s : 0x%lx\n", g_block_str[tmp->type], (size_t)tmp->addr);
		print_page(tmp, &total);
	}
	printf("total: %lu\n", total);
	MALLOC_UNLOCK();
}
