#include <stddef.h>

size_t strlen(const char *s)
{
	size_t i = 0;
	while (s[i])
		i++;
	return i;
}

void *memcpy(void *d, const void *s, size_t n)
{
	for (size_t i = 0; i < n; ++i)
		((uint8_t*)d)[i] = ((uint8_t*)s)[i];
	return d;
}
