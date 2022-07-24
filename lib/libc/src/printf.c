#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>

#define FLAG_MINUS (1 << 0)
#define FLAG_SPACE (1 << 2)
#define FLAG_ZERO  (1 << 3)
#define FLAG_PLUS  (1 << 4)
#define FLAG_SHARP (1 << 5)
#define FLAG_HH    (1 << 6)
#define FLAG_H     (1 << 7)
#define FLAG_LL    (1 << 8)
#define FLAG_L     (1 << 9)
#define FLAG_J     (1 << 10)
#define FLAG_Z     (1 << 11)
#define FLAG_T     (1 << 12)

struct arg
{
	va_list *va_arg;
	uint32_t flags;
	int width;
	int preci;
	uint8_t type;
	char j;
	char z;
};

struct printf_buf
{
	char *data;
	size_t size;
	size_t len;
};

typedef void (*print_fn_t)(struct printf_buf*, struct arg*);

static const print_fn_t g_print_fns[256];

static int parse_arg(struct arg *arg, const char *fmt, size_t *i);

static void arg_ctr(struct arg *arg, va_list *va_arg)
{
	arg->va_arg = va_arg;
	arg->flags = 0;
	arg->width = -1;
	arg->preci = -1;
	arg->type = '\0';
}

static void outchar(struct printf_buf *buf, char c)
{
	if (buf->len < buf->size)
		buf->data[buf->len] = c;
	buf->len++;
}

static void outstr(struct printf_buf *buf, const char *s)
{
	for (size_t i = 0; s[i]; ++i)
		outchar(buf, s[i]);
}

static void outchars(struct printf_buf *buf, char c, size_t n)
{
	for (size_t i = 0; i < n; ++i)
		outchar(buf, c);
}

static long long int get_int_val(struct arg *arg)
{
	if (arg->flags & FLAG_LL)
		return va_arg(*arg->va_arg, long long int);
	else if (arg->flags & FLAG_L)
		return va_arg(*arg->va_arg, long int);
	else if (arg->flags & FLAG_HH)
		return (char)va_arg(*arg->va_arg, int);
	else if (arg->flags & FLAG_H)
		return (short int)va_arg(*arg->va_arg, int);
	else if (arg->flags & FLAG_J)
		return va_arg(*arg->va_arg, intmax_t);
	else if (arg->flags & FLAG_Z)
		return va_arg(*arg->va_arg, size_t);
	else if (arg->flags & FLAG_T)
		return va_arg(*arg->va_arg, ptrdiff_t);
	return va_arg(*arg->va_arg, int);
}

static unsigned long long int get_uint_val(struct arg *arg)
{
	if (arg->flags & FLAG_LL)
		return va_arg(*arg->va_arg, unsigned long long int);
	else if (arg->flags & FLAG_L)
		return va_arg(*arg->va_arg, unsigned long int);
	else if (arg->flags & FLAG_HH)
		return (unsigned char)va_arg(*arg->va_arg, unsigned int);
	else if (arg->flags & FLAG_H)
		return (unsigned short int)va_arg(*arg->va_arg, unsigned int);
	else if (arg->flags & FLAG_J)
		return va_arg(*arg->va_arg, uintmax_t);
	else if (arg->flags & FLAG_Z)
		return va_arg(*arg->va_arg, size_t);
	else if (arg->flags & FLAG_T)
		return va_arg(*arg->va_arg, ptrdiff_t);
	return va_arg(*arg->va_arg, unsigned int);
}

static int printf_buf(struct printf_buf *buf, const char *fmt, va_list va_arg)
{
	for (size_t i = 0; fmt[i]; ++i)
	{
		if (fmt[i] == '%')
		{
			struct arg arg;
			i++;
			arg_ctr(&arg, &va_arg);
			parse_arg(&arg, fmt, &i);
			if (g_print_fns[arg.type])
				g_print_fns[arg.type](buf, &arg);
		}
		else
		{
			outchar(buf, fmt[i]);
		}
	}
	if (buf->len < buf->size)
		buf->data[buf->len] = '\0';
	else
		buf->data[buf->size - 1] = '\0';
	return buf->len;
}

int vfprintf(FILE *fp, const char *fmt, va_list va_arg)
{
	char str[1024];
	struct printf_buf buf;
	buf.data = str;
	buf.size = sizeof(str);
	buf.len = 0;
	int ret = printf_buf(&buf, fmt, va_arg);
	fwrite(buf.data, strlen(buf.data), 1, fp);
	return ret;
}

int fprintf(FILE *fp, const char *fmt, ...)
{
	va_list va_arg;
	va_start(va_arg, fmt);
	int ret = vfprintf(fp, fmt, va_arg);
	va_end(va_arg);
	return ret;
}

int vprintf(const char *fmt, va_list va_arg)
{
	return vfprintf(stdout, fmt, va_arg);
}

int printf(const char *fmt, ...)
{
	va_list va_arg;
	va_start(va_arg, fmt);
	int ret = vfprintf(stdout, fmt, va_arg);
	va_end(va_arg);
	return ret;
}

int vsnprintf(char *d, size_t n, const char *fmt, va_list va_arg)
{
	struct printf_buf buf;
	buf.data = d;
	buf.size = n;
	buf.len = 0;
	return printf_buf(&buf, fmt, va_arg);
}

int snprintf(char *d, size_t n, const char *fmt, ...)
{
	va_list va_arg;
	va_start(va_arg, fmt);
	int ret = vsnprintf(d, n, fmt, va_arg);
	va_end(va_arg);
	return ret;
}

static int parse_flags(struct arg *arg, char c)
{
	if (c == '-')
		arg->flags |= FLAG_MINUS;
	else if (c == '+')
		arg->flags |= FLAG_PLUS;
	else if (c == '0')
		arg->flags |= FLAG_ZERO;
	else if (c == '#')
		arg->flags |= FLAG_SHARP;
	else if (c == ' ')
		arg->flags |= FLAG_SPACE;
	else
		return 0;
	return 1;
}

static int parse_preci(struct arg *arg, const char *fmt, size_t *i)
{
	size_t start;
	size_t end;

	if (fmt[*i] != '.')
		return 1;
	(*i)++;
	if (fmt[*i] == '*')
	{
		arg->preci = va_arg(*arg->va_arg, int);
		(*i)++;
		return 1;
	}
	start = *i;
	while (isdigit(fmt[*i]))
		(*i)++;
	end = *i;
	if (end == start)
		return 1;
	arg->preci = atoin(&fmt[start], end - start);
	return 1;
}

static void parse_length(struct arg *arg, const char *fmt, size_t *i)
{
	if (fmt[*i] == 'h')
	{
		if (fmt[*i + 1] == 'h')
		{
			arg->flags |= FLAG_HH;
			(*i)++;
		}
		else
		{
			arg->flags |= FLAG_H;
		}
		(*i)++;
	}
	else if (fmt[*i] == 'l')
	{
		if (fmt[*i + 1] == 'l')
		{
			arg->flags |= FLAG_LL;
			(*i)++;
		}
		else
		{
			arg->flags |= FLAG_L;
		}
		(*i)++;
	}
	else if (fmt[*i] == 'j')
	{
		arg->flags |= FLAG_J;
		(*i)++;
	}
	else if (fmt[*i] == 'z')
	{
		arg->flags |= FLAG_Z;
		(*i)++;
	}
	else if (fmt[*i] == '\t')
	{
		arg->flags |= FLAG_T;
		(*i)++;
	}
}

static int parse_width(struct arg *arg, const char *fmt, size_t *i)
{
	size_t start;
	size_t end;

	if (fmt[*i] == '*')
	{
		arg->width = va_arg(*arg->va_arg, int);
		(*i)++;
		return 1;
	}
	start = *i;
	while (isdigit(fmt[*i]))
		(*i)++;
	end = *i;
	if (end == start)
		return 1;
	arg->width = atoin(&fmt[start], end - start);
	return 1;
}

static int parse_arg(struct arg *arg, const char *fmt, size_t *i)
{
	while (parse_flags(arg, fmt[*i]))
		(*i)++;
	if (!parse_width(arg, fmt, i))
		return 0;
	if (!parse_preci(arg, fmt, i))
		return 0;
	parse_length(arg, fmt, i);
	arg->type = fmt[*i];
	if (arg->flags & FLAG_MINUS)
		arg->flags &= ~FLAG_ZERO;
	return 1;
}

static void print_str(struct printf_buf *buf, struct arg *arg, const char *prefix, const char *s)
{
	size_t len = strlen(s);
	size_t prefix_len = prefix ? strlen(prefix) : 0;
	size_t pad_len;

	if (arg->width > 0 && (size_t)arg->width > len + prefix_len)
		pad_len = arg->width - len - prefix_len;
	else
		pad_len = 0;
	if (pad_len && !(arg->flags & (FLAG_ZERO | FLAG_MINUS)))
			outchars(buf, ' ', pad_len);
	if (prefix)
		outstr(buf, prefix);
	if (pad_len && (arg->flags & FLAG_ZERO))
		outchars(buf, '0', pad_len);
	outstr(buf, s);
	if (pad_len && (arg->flags & FLAG_MINUS))
		outchars(buf, ' ', pad_len);
}

static void print_c(struct printf_buf *buf, struct arg *arg)
{
	uint8_t vals[2];

	vals[0] = (unsigned char)va_arg(*arg->va_arg, int);
	vals[1] = '\0';
	print_str(buf, arg, NULL, (char*)&vals[0]);
}

static void print_d(struct printf_buf *buf, struct arg *arg)
{
	long long int val;
	char str[64];

	val = get_int_val(arg);
	lltoa(str, val);
	print_str(buf, arg, NULL, str);
}

static void print_i(struct printf_buf *buf, struct arg *arg)
{
	print_d(buf, arg);
}

static void print_o(struct printf_buf *buf, struct arg *arg)
{
	char str[64];
	unsigned long long int val;
	const char *prefix;

	val = get_uint_val(arg);
	ulltoa_base(str, val, "01234567");
	if (val && (arg->flags & FLAG_SHARP))
		prefix = "0";
	else
		prefix = NULL;
	print_str(buf, arg, prefix, str);
}

static void print_s(struct printf_buf *buf, struct arg *arg)
{
	char *str;

	str = va_arg(*arg->va_arg, char*);
	if (!str)
		str = "(null)";
	print_str(buf, arg, NULL, str);
}

static void print_u(struct printf_buf *buf, struct arg *arg)
{
	char str[64];
	unsigned long long int val;

	val = get_uint_val(arg);
	ulltoa(str, val);
	print_str(buf, arg, NULL, str);
}

static char *get_x_chars(struct arg *arg)
{
	if (arg->type == 'X')
		return "0123456789ABCDEF";
	return "0123456789abcdef";
}

static void print_x(struct printf_buf *buf, struct arg *arg)
{
	char str[64];
	unsigned long long int val;
	const char *prefix;

	val = get_uint_val(arg);
	ulltoa_base(str, val, get_x_chars(arg));
	if (val && (arg->flags & FLAG_SHARP))
		prefix = (arg->type == 'X' ? "0X" : "0x");
	else
		prefix = NULL;
	print_str(buf, arg, prefix, str);
}

static void print_X(struct printf_buf *buf, struct arg *arg)
{
	print_x(buf, arg);
}

static void print_p(struct printf_buf *buf, struct arg *arg)
{
	arg->flags |= FLAG_SHARP;
	arg->flags &= ~(FLAG_LL | FLAG_H | FLAG_HH | FLAG_Z | FLAG_J | FLAG_T);
	print_x(buf, arg);
}

static void print_mod(struct printf_buf *buf, struct arg *arg)
{
	(void)arg;
	outchar(buf, '%');
}

static const print_fn_t g_print_fns[256] =
{
	['s'] = print_s,
	['p'] = print_p,
	['d'] = print_d,
	['i'] = print_i,
	['o'] = print_o,
	['u'] = print_u,
	['x'] = print_x,
	['X'] = print_X,
	['c'] = print_c,
	['%'] = print_mod,
};
