#include "std.h"

#if 0

#define PAGE_SIZE 128

#define TINY_SIZE 128
#define SMALL_SIZE 1024

#define MALLOC_LOCK()
#define MALLOC_UNLOCK()

typedef struct page_list page_list_t;
typedef struct page page_t;

enum block_type
{
	BLOCK_TINY,
	BLOCK_SMALL,
	BLOCK_LARGE,
};

struct page
{
	enum block_type type;
	size_t len;
	void *addr;
	int blocks[PAGE_SIZE];
};

struct page_list
{
	page_t page;
	page_list_t *next;
};

page_list_t *g_pages;

page_list_t *alloc_page(enum block_type type, size_t len)
{
	t_page_list	*new;

	if (!(new = mmap(0, getpagesize_mult(len + sizeof(*new))
					, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0)))
		return (NULL);
	new->page.type = type;
	new->page.len = len + sizeof(*new);
	new->page.addr = new;
	new->page.addr += sizeof(*new);
	new->next = NULL;
	if (type == TINY || type == SMALL)
		memset(new->page.blocks, 0, sizeof(new->page.blocks);
	return new;
}

void *calloc(size_t nmemb, size_t size)
{
	char *addr = malloc(nmemb * size);
	if (addr)
		memset(addr, 0, nmemb * size);
	return addr;
}

t_page_list		*g_pages;

static int	is_page_free(t_page *page)
{
	int		i;

	i = 0;
	while (i < PAGE_SIZE)
	{
		if (page->blocks[i])
			return (0);
		++i;
	}
	return (1);
}

void		check_free_pages(t_block_type type)
{
	t_page_list		*lst;
	int				one_free;

	one_free = 0;
	lst = g_pages;
	while (lst)
	{
		if (lst->page.type == type && is_page_free(&lst->page))
		{
			if (!one_free)
				one_free = 1;
			else
			{
				remove_page(lst);
				munmap(lst, get_block_size(lst->page.type));
			}
		}
		lst = lst->next;
	}
}
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   create_new_block.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acazuc <acazuc@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2016/02/16 11:24:50 by acazuc            #+#    #+#             */
/*   Updated: 2016/09/29 16:27:37 by acazuc           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

void		*create_new_block(t_block_type type, size_t len)
{
	t_page_list		*new;

	if (!(new = alloc_page(type, type == LARGE ? len : get_page_size(type))))
		return (NULL);
	new->page.blocks[0] = 1;
	push_new_page(new);
	return (new->page.addr);
}
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   free.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acazuc <acazuc@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2016/02/16 13:11:39 by acazuc            #+#    #+#             */
/*   Updated: 2016/09/30 12:16:33 by acazuc           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

t_page_list		*g_pages;
pthread_mutex_t	g_malloc_mutex;

static int		case_large(void *addr, t_page_list *prv, t_page_list *lst)
{
	if (addr == lst->page.addr)
	{
		if (prv)
			prv->next = lst->next;
		else
			g_pages = lst->next;
		munmap(lst, lst->page.len);
		MALLOC_UNLOCK();
		return (1);
	}
	return (0);
}

static void		case_else(void *addr, t_page_list *lst)
{
	int			item;

	item = (addr - lst->page.addr) / get_block_size(lst->page.type);
	if (lst->page.addr + get_block_size(lst->page.type) * item == addr)
	{
		lst->page.blocks[item] = 0;
		check_free_pages(lst->page.type);
	}
	MALLOC_UNLOCK();
}

void			free(void *addr)
{
	t_page_list	*prv;
	t_page_list	*lst;

	if (!addr)
		return ;
	MALLOC_LOCK();
	prv = NULL;
	lst = g_pages;
	while (lst)
	{
		if (lst->page.type == LARGE && case_large(addr, prv, lst))
			return ;
		else if (lst->page.type != LARGE && addr >= lst->page.addr
				&& addr <= lst->page.addr + get_page_size(lst->page.type))
		{
			case_else(addr, lst);
			return ;
		}
		prv = lst;
		lst = lst->next;
	}
	MALLOC_UNLOCK();
}
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_block_size.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acazuc <acazuc@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2016/02/16 10:09:18 by acazuc            #+#    #+#             */
/*   Updated: 2016/02/16 10:09:59 by acazuc           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

size_t	get_block_size(t_block_type type)
{
	if (type == TINY)
		return (TINY_SIZE);
	return (SMALL_SIZE);
}
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_block_type.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acazuc <acazuc@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2016/02/15 13:40:56 by acazuc            #+#    #+#             */
/*   Updated: 2016/02/15 13:42:22 by acazuc           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

t_block_type	get_block_type(size_t len)
{
	if (len <= TINY_SIZE)
		return (TINY);
	else if (len <= SMALL_SIZE)
		return (SMALL);
	return (LARGE);
}
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_existing_block.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acazuc <acazuc@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2016/02/15 13:51:03 by acazuc            #+#    #+#             */
/*   Updated: 2016/09/29 15:26:04 by acazuc           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

t_page_list		*g_pages;

static int		get_first_free(t_page *page)
{
	int				i;

	i = 0;
	while (i < PAGE_SIZE)
	{
		if (page->blocks[i] == 0)
			return (i);
		i++;
	}
	return (-1);
}

void			*get_existing_block(t_block_type type)
{
	t_page_list		*lst;
	int				i;

	lst = g_pages;
	while (lst)
	{
		if (lst->page.type == type && (i = get_first_free(&lst->page)) != -1)
		{
			lst->page.blocks[i] = 1;
			return (lst->page.addr + i * get_block_size(type));
		}
		lst = lst->next;
	}
	return (NULL);
}
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_page_size.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acazuc <acazuc@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2016/02/15 14:57:30 by acazuc            #+#    #+#             */
/*   Updated: 2016/02/15 15:18:42 by acazuc           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

size_t		get_page_size(t_block_type type)
{
	if (type == TINY)
		return (TINY_SIZE * PAGE_SIZE);
	return (SMALL_SIZE * PAGE_SIZE);
}
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   getpagesize_mult.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acazuc <acazuc@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2016/09/29 14:35:30 by acazuc            #+#    #+#             */
/*   Updated: 2016/09/29 14:36:46 by acazuc           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

size_t	getpagesize_mult(size_t len)
{
	if (len % getpagesize() == 0)
		return (len);
	return (((len / getpagesize()) + 1) * getpagesize());
}
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   malloc.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acazuc <acazuc@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2016/02/15 13:39:20 by acazuc            #+#    #+#             */
/*   Updated: 2016/09/30 12:16:20 by acazuc           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

t_page_list		*g_pages = NULL;
pthread_mutex_t	g_malloc_mutex = PTHREAD_MUTEX_INITIALIZER;

static void		*return_enomem(void *ptr)
{
	if (ptr == NULL)
		errno = ENOMEM;
	MALLOC_UNLOCK();
	return (ptr);
}

void			*malloc(size_t len)
{
	t_block_type	type;
	void			*addr;

	MALLOC_LOCK();
	if (len == 0)
		return (return_enomem(NULL));
	type = get_block_type(len);
	if (type == LARGE || !(addr = get_existing_block(type)))
	{
		if (!(addr = create_new_block(type, len)))
			return (return_enomem(NULL));
	}
	return (return_enomem(addr));
}
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   push_new_page.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acazuc <acazuc@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2016/02/15 15:10:28 by acazuc            #+#    #+#             */
/*   Updated: 2016/02/22 10:11:21 by acazuc           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

t_page_list		*g_pages;

void	push_new_page(t_page_list *new)
{
	t_page_list		*lst;

	if (!g_pages)
		g_pages = new;
	else
	{
		lst = g_pages;
		while (lst->next)
			lst = lst->next;
		lst->next = new;
	}
}
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   realloc.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acazuc <acazuc@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2016/02/22 10:49:17 by acazuc            #+#    #+#             */
/*   Updated: 2016/09/30 12:15:45 by acazuc           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

t_page_list		*g_pages;
pthread_mutex_t	g_malloc_mutex;

static void	*return_enomem(void *addr)
{
	if (addr == NULL)
		errno = ENOMEM;
	return (addr);
}

void		*realloc_large(t_page_list *lst, void *ptr, size_t len)
{
	size_t	i;
	void	*addr;

	if (!(addr = create_new_block(LARGE, len)))
	{
		MALLOC_UNLOCK();
		return (return_enomem(NULL));
	}
	i = 0;
	while (i < len && i < lst->page.len - sizeof(*lst))
	{
		((unsigned char*)addr)[i] = ((unsigned char*)ptr)[i];
		i++;
	}
	remove_page(lst);
	munmap(lst, lst->page.len);
	MALLOC_UNLOCK();
	return (return_enomem(addr));
}

void		*realloc_small_tiny(t_page_list *lst, void *ptr, size_t len)
{
	size_t	i;
	void	*addr;

	if (len <= get_block_size(lst->page.type))
	{
		MALLOC_UNLOCK();
		return (return_enomem(ptr));
	}
	if (!(addr = create_new_block(get_block_type(len), len)))
	{
		MALLOC_UNLOCK();
		return (return_enomem(NULL));
	}
	i = 0;
	while (i < len && i < lst->page.len - sizeof(*lst))
	{
		((unsigned char*)addr)[i] = ((unsigned char*)ptr)[i];
		i++;
	}
	free(ptr);
	MALLOC_UNLOCK();
	return (return_enomem(addr));
}

void		*realloc(void *addr, size_t len)
{
	t_page_list	*lst;
	int			item;

	if (addr == NULL)
		return (malloc(len));
	MALLOC_LOCK();
	lst = g_pages;
	while (lst)
	{
		if (lst->page.type == LARGE && addr == lst->page.addr)
			return (realloc_large(lst, addr, len));
		else if (lst->page.type != LARGE && addr >= lst->page.addr
				&& addr <= lst->page.addr + get_page_size(lst->page.type))
		{
			item = (addr - lst->page.addr) / get_block_size(lst->page.type);
			if (lst->page.addr + get_block_size(lst->page.type) * item == addr)
				return (realloc_small_tiny(lst, addr, len));
			return (return_enomem(NULL));
		}
		lst = lst->next;
	}
	MALLOC_UNLOCK();
	return (return_enomem(NULL));
}
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   remove_page.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acazuc <acazuc@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2016/02/22 11:09:43 by acazuc            #+#    #+#             */
/*   Updated: 2016/02/22 11:13:27 by acazuc           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

t_page_list	*g_pages;

void	remove_page(t_page_list *page)
{
	t_page_list	*prv;
	t_page_list	*lst;

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
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   show_alloc_mem_2.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: acazuc <acazuc@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2016/02/22 10:45:57 by acazuc            #+#    #+#             */
/*   Updated: 2016/02/22 10:47:34 by acazuc           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "malloc.h"

void		putaddrchar(char c)
{
	if (c > 9)
		ft_putchar(c - 10 + 'a');
	else
		ft_putchar(c + '0');
}

void		putaddr(size_t addr)
{
	if ((size_t)addr > 15)
	{
		putaddr(addr / 16);
		putaddr(addr % 16);
	}
	else
		putaddrchar((char)(addr % 16));
}


static void			print_block(size_t start, size_t end, size_t len)
{
	ft_putstr("0x");
	putaddr(start);
	ft_putstr(" - 0x");
	putaddr(end);
	ft_putstr(" : ");
	ft_putnbr(len);
	ft_putendl(" octets");
}

static void			c_e(void **start, void *end, size_t *total)
{
	*total += end - *start;
	print_block((size_t)*start, (size_t)end, (size_t)(end - *start));
	*start = NULL;
}

static void			print_page(t_page *page, size_t *total)
{
	void	*start;
	void	*end;
	int		i;

	if (page->type == LARGE)
	{
		*total += page->len - sizeof(t_page_list);
		print_block((size_t)page->addr, (size_t)(page->addr + page->len)
				, page->len);
		return ;
	}
	start = NULL;
	end = NULL;
	i = -1;
	while (++i < PAGE_SIZE)
	{
		if (page->blocks[i] == 1 && !start)
			start = page->addr + i * get_block_size(page->type);
		else if (page->blocks[i] == 0 && start)
			c_e(&start, page->addr + i * get_block_size(page->type), total);
	}
	if (start)
		c_e(&start, page->addr + i * get_block_size(page->type), total);
}

static t_page_list	*try_push(size_t min)
{
	t_page_list		*lowest;
	t_page_list		*lst;
	size_t			lowest_val;

	lowest = NULL;
	lst = g_pages;
	lowest_val = -1;
	while (lst)
	{
		if ((lowest_val == 0 || (size_t)lst < lowest_val) && (size_t)lst > min)
		{
			lowest = lst;
			lowest_val = (size_t)lowest;
		}
		lst = lst->next;
	}
	return (lowest);
}

void show_alloc_mem(void)
{
	page_list_t *tmp;
	size_t total;

	MALLOC_LOCK();
	total = 0;
	tmp = NULL;
	while (1)
	{
		if (!(tmp = try_push((size_t)tmp)))
			break ;
		if (tmp->page.type == TINY)
			ft_putstr("TINY");
		else
			ft_putstr(tmp->page.type == SMALL ? "SMALL" : "LARGE");
		ft_putstr(" : 0x");
		putaddr((size_t)tmp->page.addr);
		ft_putchar('\n');
		print_page(&tmp->page, &total);
	}
	ft_putstr("Total: ");
	ft_putul(total);
	ft_putchar('\n');
	MALLOC_UNLOCK();
}

#endif
