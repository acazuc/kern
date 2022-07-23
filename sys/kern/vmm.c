#include <sys/vmm.h>
#include <sys/std.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <arch.h>

/* that's a bit of sad code, it somehow should be simplified */
int vmm_get_free_range(struct vmm_region *region, size_t addr, size_t size, size_t *ret)
{
	if (addr && (addr < region->addr || addr + size > region->addr + region->size))
		return ENOMEM;
	if (TAILQ_EMPTY(&region->ranges))
	{
		if (!addr || addr == region->addr)
		{
			region->range_0.addr = region->addr + size;
			region->range_0.size = region->size - size;
			TAILQ_INSERT_HEAD(&region->ranges, &region->range_0, chain);
			*ret = region->addr;
			goto end;
		}
		if (addr + size == region->addr + region->size)
		{
			region->range_0.addr = region->addr;
			region->range_0.size = region->size - size;
			TAILQ_INSERT_HEAD(&region->ranges, &region->range_0, chain);
			*ret = region->addr + region->size - size;
			goto end;
		}
		struct vmm_range *range = malloc(sizeof(*range), 0);
		assert(range, "can't allocate new range\n");
		range->addr = region->addr;
		range->size = addr - region->addr;
		TAILQ_INSERT_HEAD(&region->ranges, range, chain);
		region->range_0.addr = addr + size;
		region->range_0.size = region->addr + region->size - region->range_0.addr;
		TAILQ_INSERT_HEAD(&region->ranges, &region->range_0, chain);
		*ret = addr;
		goto end;
	}
	struct vmm_range *item;
	if (addr)
	{
		TAILQ_FOREACH(item, &region->ranges, chain)
		{
			if (addr < item->addr)
				return ENOMEM;
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
				*ret = addr;
				goto end;
			}
			if (addr == item->addr)
			{
				item->addr += size;
				item->size -= size;
				*ret = addr;
				goto end;
			}
			if (addr + size == item->addr + item->size)
			{
				item->size -= size;
				*ret = addr;
				goto end;
			}
			struct vmm_range *newr = malloc(sizeof(*newr), 0);
			assert(newr, "can't allocate new range\n");
			newr->addr = addr + size;
			newr->size = item->size - (addr + size - item->addr);
			TAILQ_INSERT_AFTER(&region->ranges, item, newr, chain);
			item->size = addr - item->addr;
			*ret = addr;
			goto end;
		}
	}
	else
	{
		TAILQ_FOREACH(item, &region->ranges, chain)
		{
			if (item->size < size)
				continue;
			*ret = item->addr;
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
			goto end;
		}
	}
	return ENOMEM;

end:
	return 0;
}

/* same sad code sa above */
void vmm_set_free_range(struct vmm_region *region, size_t addr, size_t size)
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
			if (prev->addr + prev->size != addr)
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

