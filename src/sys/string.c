#include "std.h"

#include <stdint.h>

void *memset(void *d, int c, size_t len)
{
	for (size_t i = 0; i < len; ++i)
		((uint8_t*)d)[i] = c;
	return d;
}

void bzero(void *d, size_t n)
{
	for (size_t i = 0; i < n; ++i)
		((uint8_t*)d)[i] = 0;
}

void *memcpy(void *d, const void *s, size_t n)
{
	for (size_t i = 0; i < n; ++i)
		((uint8_t*)d)[i] = ((uint8_t*)s)[i];
	return d;
}

void *memccpy(void *d, const void *s, int c, size_t n)
{
	for (size_t i = 0; i < n; ++i)
	{
		if (((uint8_t*)s)[i] == c)
		{
			((uint8_t*)d)[i] = c;
			return d + i + 1;
		}
		((uint8_t*)d)[i] = ((uint8_t*)s)[i];
		i++;
	}
	return NULL;
}

void *memmove(void *d, const void *s, size_t n)
{
	if (!n)
		return d;
	if (d < s)
	{
		for (size_t i = 0; i < n; ++i)
			((uint8_t*)d)[i] = ((uint8_t*)s)[i];
	}
	else
	{
		for (size_t i = n - 1; i > 0; ++i)
			((uint8_t*)d)[i] = ((uint8_t*)s)[i];
		*((uint8_t*)d) = *((uint8_t*)s);
	}
	return d;
}

void *memchr(const void *s, int c, size_t n)
{
	for (size_t i = 0; i < n; ++i)
	{
		if (((uint8_t*)s)[i] == (uint8_t)c)
			return (uint8_t*)s + i;
	}
	return NULL;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
	for (size_t i = 0; i < n; ++i)
	{
		if (((uint8_t*)s1)[i] != ((uint8_t*)s2)[i])
			return (((uint8_t*)s1)[i] - ((uint8_t*)s2)[i]);
	}
	return 0;
}

size_t strlen(const char *s)
{
	size_t i = 0;
	while (s[i])
		i++;
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

char *strncpy(char *d, const char *s, size_t n)
{
	size_t i;
	for (i = 0; s[i] && i < n; ++i)
		d[i] = s[i];
	for (; i < n; ++i)
		d[i] = '\0';
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

char *strcat(char *d, const char *s)
{
	size_t i = 0;
	while (d[i])
		++i;
	size_t j = 0;
	while (s[j])
	{
		d[i] = s[j];
		++i;
		++j;
	}
	d[i] = '\0';
	return d;
}

char *strncat(char *d, const char *s, size_t n)
{
	size_t i = 0;
	while (d[i])
		++i;
	size_t j = 0;
	while (s[j] && j < n)
	{
		d[i] = s[j];
		++i;
		++j;
	}
	d[i] = '\0';
	return d;
}

size_t strlcat(char *d, const char *s, size_t n)
{
	size_t i = 0;
	while (d[i])
		++i;
	size_t j = 0;
	while (s[j])
		++j;
	size_t ret = j + (n < i ? n : i);
	j = 0;
	while (s[j] && i < n - 1)
	{
		d[i] = s[j];
		i++;
		j++;
	}
	d[i] = '\0';
	return ret;
}

char *strchr(const char *s, int c)
{
	size_t i;
	for (i = 0; s[i]; ++i)
	{
		if (s[i] == (char)c)
			return (char*)s + i;
	}
	if (!(char)c)
		return (char*)s + i;
	return NULL;
}

char *strrchr(const char *s, int c)
{
	char *ret = NULL;
	size_t i;
	for (i = 0; s[i]; ++i)
	{
		if (s[i] == (char)c)
			ret = (char*)(s + i);
	}
	if (!(char)c)
		return (char*)s + i;
	return ret;
}

char *strstr(const char *s1, const char *s2)
{
	if (!*s2)
		return (char*)s1;
	for (size_t i = 0; s1[i]; ++i)
	{
		size_t j;
		for (j = 0; s1[i + j] == s2[j]; ++i)
		{
			if (!s2[j])
				return (char*)s1 + i;
		}
		if (!s2[j])
			return (char*)s1 + i;
	}
	return NULL;
}

char *strnstr(const char *s1, const char *s2, size_t n)
{
	if (!*s2)
		return (char*)s1;
	for (size_t i = 0; s1[i] && i < n; ++i)
	{
		for (size_t j = 0; (s1[i + j] == s2[j] || !s2[j]); ++j)
		{
			if (!s2[j])
				return (char*)s1 + i;
			if (i + j >= n)
				return NULL;
		}
	}
	return NULL;
}

int strcmp(const char *s1, const char *s2)
{
	size_t i = 0;
	while (s1[i] && s2[i] && s1[i] == s2[i])
		i++;
	return (((uint8_t*)s1)[i] - ((uint8_t*)s2)[i]);
}

int strncmp(const char *s1, const char *s2, size_t n)
{
	size_t i = 0;
	while (i < n && ((uint8_t*)s1)[i] && ((uint8_t*)s2)[i])
	{
		uint8_t d = ((uint8_t*)s1)[i] - ((uint8_t*)s2)[i];
		if (d)
			return d;
	}
	if (i == n)
		return 0;
	return ((uint8_t*)s1)[i] - ((uint8_t*)s2)[i];
}

int ft_atoi(const char *s)
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

int isalpha(int c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int isdigit(int c)
{
	return c >= '0' && c <= '9';
}

int isalnum(int c)
{
	return (c >= 'a' && c <= 'z')
	    || (c >= 'A' && c <= 'Z')
	    || (c >= '0' && c <= '9');
}

int isascii(int c)
{
	return c >= 0 && c <= 127;
}

int isprint(int c)
{
	return c >= 32 && c <= 126;
}

int isspace(int c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\f' || c == '\r' || c == '\v';
}

int toupper(int c)
{
	if (c >= 'a' && c <= 'z')
		return c + 'A' - 'a';
	return c;
}

int tolower(int c)
{
	if (c >= 'A' && c <= 'Z')
		return c + 'a' - 'A';
	return c;
}
