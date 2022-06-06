#include "std.h"

#include <stdarg.h>
#include <stdint.h>

typedef struct flags
{
	char minus;
	char space;
	char zero;
	char plus;
	char sharp;
} flags_t;

typedef struct arg
{
	flags_t flags;
	va_list *va_lst;
	int width;
	int preci;
	char type;
	char h;
	char hh;
	char l;
	char ll;
	char j;
	char z;
} arg_t;

void flags_ctr(flags_t *flags)
{
	flags->minus = 0;
	flags->space = 0;
	flags->zero = 0;
	flags->plus = 0;
	flags->sharp = 0;
}

void arg_ctr(arg_t *arg, va_list *lst)
{
	flags_ctr(&arg->flags);
	arg->va_lst = lst;
	arg->width = -1;
	arg->preci = -1;
	arg->type = '\0';
	arg->l = 0;
	arg->ll = 0;
	arg->h = 0;
	arg->hh = 0;
	arg->j = 0;
	arg->z = 0;
}

static size_t	get_size(long long int n)
{
	size_t					size;

	size = n < 0 ? 2 : 1;
	n = n < 0 ? -n : n;
	while (n > 0)
	{
		size++;
		n /= 10;
	}
	return (size);
}

char			*ft_ltoa(long long int n)
{
	char					*result;
	size_t					size;
	unsigned long long int	j;
	unsigned long long int	i;
	unsigned long long int	nb;

	if (n == 0)
		return (ft_strdup("0"));
	nb = n < 0 ? -n : n;
	size = get_size(n);
	if (!(result = malloc(sizeof(result) * size)))
		return (result);
	if (n < 0)
		result[0] = '-';
	j = 1;
	i = 1;
	while (nb / j > 0)
	{
		result[size - i++ - 1] = (nb / j) % 10 + '0';
		j = j * 10;
	}
	result[size - 1] = '\0';
	return (result);
}

char	*ft_strjoin(char const *s1, char const *s2)
{
	char	*result;
	size_t	size;
	size_t	i;
	size_t	j;

	i = 0;
	while (s1[i])
		i++;
	size = i;
	i = 0;
	while (s2[i])
		i++;
	size = size + i + 1;
	result = malloc(sizeof(*result) * size);
	if (!result)
		return (result);
	i = -1;
	j = -1;
	while (s1[++i])
		result[++j] = s1[i];
	i = -1;
	while (s2[++i])
		result[++j] = s2[i];
	result[++j] = '\0';
	return (result);
}

char	*ft_strjoin_free2(char *s1, char *s2)
{
	char	*result;

	result = ft_strjoin(s1, s2);
	free(s2);
	return (result);
}

char	*ft_strsub(char const *s, unsigned int start, size_t len)
{
	char	*result;
	size_t	i;

	result = malloc(sizeof(*result) * (len + 1));
	if (!result)
		return (result);
	i = 0;
	while (i < len)
	{
		result[i] = s[start + i];
		i++;
	}
	result[len] = '\0';
	return (result);
}

static size_t	get_size(unsigned long long int n, char *base)
{
	size_t					size;

	size = 1;
	while (n > 0)
	{
		size++;
		n /= ft_strlen(base);
	}
	return (size);
}

char			*ft_ultoa_base(unsigned long long int n, char *base)
{
	char					*result;
	size_t					size;
	unsigned long long int	j;
	unsigned long long int	i;
	unsigned long long int	nb;

	if (!base || ft_strlen(base) < 2)
		return (NULL);
	if (n == 0)
		return (ft_strdup("0"));
	nb = n;
	size = get_size(n, base);
	if (!(result = malloc(sizeof(result) * size)))
		return (result);
	j = 1;
	i = 1;
	while (nb / j > 0)
	{
		result[size - i++ - 1] = base[(nb / j) % ft_strlen(base)];
		j = j * ft_strlen(base);
	}
	result[size - 1] = '\0';
	return (result);
}

static size_t	get_size(unsigned long long int n)
{
	size_t					size;

	size = 1;
	while (n > 0)
	{
		size++;
		n /= 10;
	}
	return (size);
}

char			*ft_ultoa(unsigned long long int n)
{
	char					*result;
	size_t					size;
	unsigned long long int	j;
	unsigned long long int	i;
	unsigned long long int	nb;

	if (n == 0)
		return (ft_strdup("0"));
	nb = n;
	size = get_size(n);
	if (!(result = malloc(sizeof(result) * size)))
		return (result);
	j = 1;
	i = 1;
	while (nb / j > 0)
	{
		result[size - i++ - 1] = (nb / j) % 10 + '0';
		j = j * 10;
	}
	result[size - 1] = '\0';
	return (result);
}

size_t	ft_wstrlen(wchar_t const *s)
{
	size_t	len;

	len = 0;
	while (s[len])
		len++;
	return (len);
}

wchar_t	*ft_wstrsub(wchar_t const *s, unsigned int start, size_t len)
{
	wchar_t	*result;
	size_t	i;

	result = malloc(sizeof(*result) * (len + 1));
	if (!result)
		return (result);
	i = 0;
	while (i < len)
	{
		result[i] = s[start + i];
		i++;
	}
	result[len] = L'\0';
	return (result);
}

int printf(const char *fmt, ...)
{
	va_list va_arg;

	va_start(va_arg, fmt);
	for (size_t i = 0; fmt[i]; ++i)
	{
		if (fmt[i] == '%')
		{
			arg_t *arg;
			i++;
			parse_arg(str, &i, &va_arg);
			print_argument(argument);
			argument_free(argument);
			continue;
		}
		else
		{
			ft_putchar(str[i]);
		}
	}
	va_end(va_arg);
}

static void	parse_flags(t_flags *flags, char *str, size_t *i)
{
	if (str[*i] == '-')
		flags->minus = 1;
	else if (str[*i] == '+')
		flags->plus = 1;
	else if (str[*i] == '0')
		flags->zero = 1;
	else if (str[*i] == '#')
		flags->sharp = 1;
	else if (str[*i] == ' ')
		flags->space = 1;
	if (str[*i] == '-' || str[*i] == '+' || str[*i] == '0' || str[*i] == '#'
			|| str[*i] == ' ')
	{
		(*i)++;
		parse_flags(flags, str, i);
	}
}

static int	parse_preci(t_argument *argument, char *str, size_t *i)
{
	size_t	start;
	size_t	end;
	char	*result;

	if (str[*i] != '.')
		return (1);
	(*i)++;
	start = *i;
	while (str[*i] >= '0' && str[*i] <= '9')
		(*i)++;
	end = *i;
	if (end == start)
		return (1);
	if (!(result = ft_strsub(str, start, end - start)))
		return (0);
	argument->preci = ft_atoi(result);
	return (1);
}

t_argument	*parse_arg(char *str, size_t *i, va_list *lst)
{
	t_argument	*argument;

	if (!(argument = argument_create(lst)))
		return (NULL);
	parse_flags(argument->flags, str, i);
	if (!parse_width(argument, str, i))
	{
		argument_free(argument);
		return (NULL);
	}
	if (!parse_preci(argument, str, i))
	{
		argument_free(argument);
		return (NULL);
	}
	parse_length(argument, str, i);
	argument->type = str[*i];
	return (argument);
}

static void	parse_h(t_argument *argument, char *str, size_t *i)
{
	if (str[*i + 1] == 'h')
	{
		argument->hh = 1;
		(*i)++;
	}
	else
		argument->h = 1;
	(*i)++;
}

static void	parse_l(t_argument *argument, char *str, size_t *i)
{
	if (str[*i + 1] == 'l')
	{
		argument->ll = 1;
		(*i)++;
	}
	else
		argument->l = 1;
	(*i)++;
}

void		parse_length(t_argument *argument, char *str, size_t *i)
{
	if (str[*i] == 'h')
		parse_h(argument, str, i);
	else if (str[*i] == 'l')
		parse_l(argument, str, i);
	else if (str[*i] == 'j')
	{
		argument->j = 1;
		(*i)++;
	}
	else if (str[*i] == 'z')
	{
		argument->z = 1;
		(*i)++;
	}
}

int				parse_width(t_argument *argument, char *str, size_t *i)
{
	size_t	start;
	size_t	end;
	char	*result;

	start = *i;
	while (str[*i] >= '0' && str[*i] <= '9')
		(*i)++;
	end = *i;
	if (end == start)
		return (1);
	if (!(result = ft_strsub(str, start, end - start)))
		return (0);
	argument->width = ft_atoi(result);
	return (1);
}

static void		print_argument_2(t_argument *argument)
{
	if (argument->type == 'X')
		print_argument_x_caps(argument);
	else if (argument->type == 'c')
		print_argument_c(argument);
	else if (argument->type == 'C')
		print_argument_c_caps(argument);
	else if (argument->type == '%')
		print_argument_percent(argument);
}

void			print_argument(t_argument *argument)
{
	if (argument->type == 's')
		print_argument_s(argument);
	else if (argument->type == 'S')
		print_argument_s_caps(argument);
	else if (argument->type == 'p')
		print_argument_p(argument);
	else if (argument->type == 'd')
		print_argument_d(argument);
	else if (argument->type == 'D')
		print_argument_d_caps(argument);
	else if (argument->type == 'i')
		print_argument_i(argument);
	else if (argument->type == 'o')
		print_argument_o(argument);
	else if (argument->type == 'O')
		print_argument_o_caps(argument);
	else if (argument->type == 'u')
		print_argument_u(argument);
	else if (argument->type == 'U')
		print_argument_u_caps(argument);
	else if (argument->type == 'x')
		print_argument_x(argument);
	else
		print_argument_2(argument);
}

void	print_argument_c(t_argument *argument)
{
	size_t	ret;
	wint_t	val;

	if (argument->l)
		val = va_arg(*argument->va_lst, wint_t);
	else
		val = (unsigned char)va_arg(*argument->va_lst, int);
	if (argument->width > 0 && !argument->flags->minus)
		print_spaces(argument->width - 1);
	if (argument->l)
		ft_putwchar(val);
	else
		ft_putchar(val);
	if (argument->width > 0 && argument->flags->minus)
		print_spaces(argument->width - 1);
	(void)ret;
}

void	print_argument_c_caps(t_argument *argument)
{
	argument->ll = 0;
	argument->l = 1;
	argument->h = 0;
	argument->hh = 0;
	argument->z = 0;
	argument->j = 0;
	print_argument_c(argument);
}

static long long int	get_val(t_argument *argument)
{
	if (argument->ll)
		return (va_arg(*argument->va_lst, long long int));
	else if (argument->l)
		return (va_arg(*argument->va_lst, long int));
	else if (argument->hh)
		return ((signed char)va_arg(*argument->va_lst, int));
	else if (argument->h)
		return ((short int)va_arg(*argument->va_lst, int));
	return (va_arg(*argument->va_lst, int));
}

void	print_argument_d(t_argument *argument)
{
	size_t			len;
	char			*str;
	long long int	val;

	val = get_val(argument);
	if (!(str = ft_ltoa(val)))
		return ;
	if (argument->flags->plus && val >= 0)
	{
		if (!(str = ft_strjoin_free2("+", str)))
			return ;
	}
	len = ft_strlen(str);
	if (!argument->flags->minus)
		print_argument_spaces(argument, len);
	if (argument->preci > 0 && (size_t)argument->preci > len)
		print_zeros(argument->preci - len);
	ft_putstr(str);
	if (argument->flags->minus)
		print_argument_spaces(argument, len);
}

void	print_argument_d_caps(t_argument *argument)
{
	argument->l = 1;
	argument->ll = 0;
	argument->h = 0;
	argument->hh = 0;
	argument->j = 0;
	print_argument_d(argument);
}

void	print_argument_i(t_argument *argument)
{
	print_argument_d(argument);
}

static unsigned long long int	get_val(t_argument *argument)
{
	if (argument->ll)
		return (va_arg(*argument->va_lst, unsigned long long int));
	else if (argument->l)
		return (va_arg(*argument->va_lst, unsigned long int));
	else if (argument->hh)
		return ((unsigned char)va_arg(*argument->va_lst, unsigned int));
	else if (argument->h)
		return ((unsigned short int)va_arg(*argument->va_lst, unsigned int));
	return (va_arg(*argument->va_lst, unsigned int));
}

void	print_argument_o(t_argument *argument)
{
	size_t					len;
	char					*str;
	unsigned long long int	val;

	val = get_val(argument);
	if (!(str = ft_ultoa_base(val, "01234567")))
		return ;
	if (argument->flags->sharp && !(str = ft_strjoin_free2("0", str)))
		return ;
	len = ft_strlen(str);
	if (!argument->flags->minus && argument->width > 0
			&& ((argument->preci <= 0 && (size_t)argument->width > len)
				|| (argument->preci >= 1 && (size_t)argument->width > MAX((size_t)argument->preci, len))))
		print_spaces(argument->width - (argument->preci <= 0 ? len : MAX((size_t)argument->preci, len)));
	if (argument->preci > 0 && (size_t)argument->preci > len)
		print_zeros(argument->preci - len);
	ft_putstr(str);
	if (argument->flags->minus && argument->width > 0
			&& ((argument->preci <= 0 && (size_t)argument->width > len)
				|| (argument->preci >= 1 && (size_t)argument->width > MAX((size_t)argument->preci, len))))
		print_spaces(argument->width - (argument->preci <= 0 ? len : MAX((size_t)argument->preci, len)));
}

void	print_argument_o_caps(t_argument *argument)
{
	argument->ll = 0;
	argument->l = 1;
	argument->hh = 0;
	argument->h = 0;
	argument->j = 0;
	argument->z = 0;
	print_argument_o(argument);
}

void	print_argument_p(t_argument *argument)
{
	argument->flags->sharp = 1;
	argument->l = 1;
	argument->ll = 0;
	argument->h = 0;
	argument->hh = 0;
	argument->z = 0;
	argument->j = 0;
	print_argument_x(argument);
}

void	print_argument_percent(t_argument *argument)
{
	(void)argument;
}

static void	print_wstr(t_argument *argument)
{
	size_t	len;
	wchar_t	*str;
	char	cut;

	str = va_arg(*argument->va_lst, wchar_t*);
	if (!str)
		str = L"(null)";
	len = ft_wstrlen(str);
	cut = 0;
	if ((size_t)argument->preci < len)
	{
		cut = 1;
		if (!(str = ft_wstrsub(str, 0, argument->preci)))
			return ;
	}
	len = ft_wstrlen(str);
	if (!argument->flags->minus && argument->width > 0 && (size_t)argument->width > len)
		print_spaces(argument->width - len);
	ft_putwstr(str);
	if (argument->flags->minus && argument->width > 0 && (size_t)argument->width > len)
		print_spaces(argument->width - len);
	if (cut)
		free(str);
}

void	print_argument_s(t_argument *argument)
{
	size_t	len;
	char	*str;
	char	cut;

	if (argument->l)
		print_wstr(argument);
	str = va_arg(*argument->va_lst, char*);
	if (!str)
		str = "(null)";
	len = ft_strlen(str);
	cut = 0;
	if ((size_t)argument->preci < len)
	{
		cut = 1;
		if (!(str = ft_strsub(str, 0, argument->preci)))
			return ;
	}
	len = ft_strlen(str);
	if (!argument->flags->minus && argument->width > 0 && (size_t)argument->width > len)
		print_spaces(argument->width - len);
	ft_putstr(str);
	if (argument->flags->minus && argument->width > 0 && (size_t)argument->width > len)
		print_spaces(argument->width - len);
	if (cut)
		free(str);
}

void	print_argument_s_caps(t_argument *argument)
{
	argument->ll = 0;
	argument->l = 1;
	argument->h = 0;
	argument->hh = 0;
	argument->j = 0;
	argument->z = 0;
	print_argument_s(argument);
}

void	print_argument_spaces(t_argument *arg, size_t len)
{
	size_t	preci;
	size_t	width;

	preci = (size_t)arg->preci;
	width = (size_t)arg->width;
	if (arg->width > 0 && ((arg->preci <= 0 && width > MAX(preci, len))))
		print_spaces(arg->width - (arg->preci <= 0 ? len : MAX(preci, len)));
}

static unsigned long long int	get_val(t_argument *argument)
{
	if (argument->ll)
		return (va_arg(*argument->va_lst, unsigned long long int));
	else if (argument->l)
		return (va_arg(*argument->va_lst, unsigned long int));
	else if (argument->hh)
		return ((unsigned char)va_arg(*argument->va_lst, unsigned int));
	else if (argument->h)
		return ((unsigned short int)va_arg(*argument->va_lst, unsigned int));
	return (va_arg(*argument->va_lst, unsigned int));
}

void	print_argument_u(t_argument *argument)
{
	size_t					len;
	char					*str;
	unsigned long long int	val;

	val = get_val(argument);
	if (!(str = ft_ultoa(val)))
		return ;
	len = ft_strlen(str);
	if (!argument->flags->minus && argument->width > 0
			&& ((argument->preci <= 0 && (size_t)argument->width > len)
				|| (argument->preci >= 1 && (size_t)argument->width > MAX((size_t)argument->preci, len))))
		print_spaces(argument->width - (argument->preci <= 0 ? len : MAX((size_t)argument->preci, len)));
	if (argument->preci > 0 && (size_t)argument->preci > len)
		print_zeros(argument->preci - len);
	ft_putstr(str);
	if (argument->flags->minus && argument->width > 0
			&& ((argument->preci <= 0 && (size_t)argument->width > len)
				|| (argument->preci >= 1 && (size_t)argument->width > MAX((size_t)argument->preci, len))))
		print_spaces(argument->width - (argument->preci <= 0 ? len : MAX((size_t)argument->preci, len)));
}

void	print_argument_u_caps(t_argument *argument)
{
	argument->l = 1;
	argument->ll = 0;
	argument->h = 0;
	argument->hh = 0;
	argument->j = 0;
	argument->z = 0;
	print_argument_u(argument);
}

static char		*get_chars(t_argument *argument)
{
	if (argument->type == 'X')
		return ("0123456789ABCDEF");
	return ("0123456789abcdef");
}

static unsigned long long int	get_val(t_argument *argument)
{
	if (argument->ll)
		return (va_arg(*argument->va_lst, unsigned long long int));
	else if (argument->l)
		return (va_arg(*argument->va_lst, unsigned long int));
	else if (argument->hh)
		return ((unsigned char)va_arg(*argument->va_lst, unsigned int));
	else if (argument->h)
		return ((unsigned short int)va_arg(*argument->va_lst, unsigned int));
	return (va_arg(*argument->va_lst, unsigned int));
}

void	print_argument_x(t_argument *argument)
{
	size_t					len;
	char					*str;
	unsigned long long int	val;

	val = get_val(argument);
	if (!(str = ft_ultoa_base(val, get_chars(argument))))
		return ;
	if (argument->flags->sharp && !(str = ft_strjoin_free2("0x", str)))
		return ;
	len = ft_strlen(str);
	if (!argument->flags->minus && argument->width > 0
			&& ((argument->preci <= 0 && (size_t)argument->width > len)
				|| (argument->preci >= 1 && (size_t)argument->width > MAX((size_t)argument->preci, len))))
		print_spaces(argument->width - (argument->preci <= 0 ? len : MAX((size_t)argument->preci, len)));
	if (argument->preci > 0 && (size_t)argument->preci > len)
		print_zeros(argument->preci - len);
	ft_putstr(str);
	if (argument->flags->minus && argument->width > 0
			&& ((argument->preci <= 0 && (size_t)argument->width > len)
				|| (argument->preci >= 1 && (size_t)argument->width > MAX((size_t)argument->preci, len))))
		print_spaces(argument->width - (argument->preci <= 0 ? len : MAX((size_t)argument->preci, len)));
}

void	print_argument_x_caps(t_argument *argument)
{
	print_argument_x(argument);
}

void	print_spaces(size_t len)
{
	size_t	count;

	count = 0;
	while (count < len)
	{
		ft_putchar(' ');
		count++;
	}
}

void	print_zeros(size_t i)
{
	size_t	count;

	count = 0;
	while (count < i)
	{
		ft_putchar('0');
		count++;
	}
}
