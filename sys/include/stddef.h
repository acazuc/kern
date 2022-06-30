#ifndef STDDEF_H
#define STDDEF_H

#include <stdint.h>

#define NULL ((void*)0)

#if defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)

typedef uint32_t size_t;
typedef int32_t ssize_t;

#else
# error "unknown arch"
#endif

#endif
