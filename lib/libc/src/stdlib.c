#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>

int atoi(const char *s)
{
	char is_neg = 0;
	size_t i = 0;
	while (s[i] && isspace(s[i]))
		i++;
	if (s[i] == '-')
		is_neg = 1;
	if (s[i] == '+' || s[i] == '-')
		i++;
	int result = 0;
	while (s[i] == '0')
		i++;
	while (s[i])
	{
		if (s[i] >= '0' && s[i++] <= '9')
			result = result * 10 + s[i - 1] - '0';
		else
			return is_neg ? -result : result;
	}
	return is_neg ? -result : result;
}

int atoin(const char *s, size_t n)
{
	char is_neg = 0;
	size_t i = 0;
	while (s[i] && i < n && isspace(s[i]))
		i++;
	if (i >= n)
		return 0;
	if (s[i] == '-')
		is_neg = 1;
	if (s[i] == '+' || s[i] == '-')
		i++;
	if (i >= n)
		return 0;
	while (s[i] == '0' && i < n)
		i++;
	if (i >= n)
		return 0;
	int result = 0;
	while (s[i] && i < n)
	{
		if (s[i] >= '0' && s[i++] <= '9')
			result = result * 10 + s[i - 1] - '0';
		else
			return is_neg ? -result : result;
	}
	return is_neg ? -result : result;
}

static size_t lltoa_get_size(long long int n, size_t base_len)
{
	size_t size = 1;
	if (n == LLONG_MIN)
	{
		n = 0 - n / base_len;
		size += 2;
	}
	else if (n < 0)
	{
		size++;
		n = -n;
	}
	while (n > 0)
	{
		size++;
		n /= base_len;
	}
	return size;
}

void lltoa_base(char *d, long long int n, const char *base)
{
	size_t size;
	size_t base_len;
	size_t i;
	unsigned long long int nb;

	if (!n)
	{
		strcpy(d, "0");
		return;
	}
	nb = n < 0 ? (unsigned long long int)(-(n + 1)) + 1 : (unsigned long long int)n;
	base_len = strlen(base);
	size = lltoa_get_size(n, base_len);
	if (n < 0)
		d[0] = '-';
	i = 2;
	while (nb > 0)
	{
		d[size - i] = base[nb % base_len];
		nb /= base_len;
		++i;
	}
	d[size - 1] = '\0';
}

void lltoa(char *d, long long int n)
{
	lltoa_base(d, n, "0123456789");
}

static size_t ulltoa_get_size(unsigned long long int n, size_t base_len)
{
	size_t size = 1;
	while (n > 0)
	{
		size++;
		n /= base_len;
	}
	return size;
}

void ulltoa_base(char *d, unsigned long long int n, const char *base)
{
	size_t size;
	size_t base_len;
	size_t i;
	unsigned long long int nb;

	if (!n)
	{
		strcpy(d, "0");
		return;
	}
	nb = n;
	base_len = strlen(base);
	size = ulltoa_get_size(n, base_len);
	i = 2;
	while (nb > 0)
	{
		d[size - i] = base[nb % base_len];
		nb /= base_len;
		++i;
	}
	d[size - 1] = '\0';
}

void ulltoa(char *d, unsigned long long int n)
{
	return ulltoa_base(d, n, "0123456789");
}

void exit(int status)
{
	/* XXX */
	_exit(status);
}
