#include <ld.h>

extern int g_stdout;

ssize_t write(int fd, const void *buffer, size_t count);

size_t strlen(const char *s)
{
	size_t i = 0;
	while (s[i])
		i++;
	return i;
}

void *memset(void *d, int c, size_t len)
{
	for (size_t i = 0; i < len; ++i)
		((uint8_t*)d)[i] = c;
	return d;
}

size_t strlcpy(char *d, const char *s, size_t n)
{
	size_t i;
	for (i = 0; s[i] && i < n - 1; ++i)
		d[i] = s[i];
	d[i] = '\0';
	while (s[i])
		++i;
	return i;
}

char *strcpy(char *d, const char *s)
{
	size_t i;
	for (i = 0; s[i]; ++i)
		d[i] = s[i];
	d[i] = '\0';
	return d;
}

int strcmp(const char *s1, const char *s2)
{
	size_t i = 0;
	while (s1[i] && s2[i] && s1[i] == s2[i])
		i++;
	return (((uint8_t*)s1)[i] - ((uint8_t*)s2)[i]);
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

void lltoax(char *d, long long int n)
{
	lltoa_base(d, n, "0123456789abcdef");
}

void puts(const char *s)
{
	write(g_stdout, s, strlen(s));
}

void putx(long long n)
{
	char str[64];
	lltoax(str, n);
	puts(str);
}

void puti(long long n)
{
	char str[64];
	lltoa(str, n);
	puts(str);
}
