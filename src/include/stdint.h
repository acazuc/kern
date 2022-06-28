#ifndef STDINT_H
#define STDINT_H

#if defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed long int32_t;
typedef unsigned long uint32_t;
typedef signed long long int64_t;
typedef unsigned long long uint64_t;
typedef unsigned long int uintptr_t;
typedef long int intptr_t;
typedef long long intmax_t;
typedef unsigned long long uintmax_t;
typedef long ptrdiff_t;

#else
# error "unknown arch"
#endif

#endif
