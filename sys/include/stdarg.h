#ifndef STDARG_H
#define STDARG_H

#if defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)

typedef char *va_list;

#define va_rsize(T) ((sizeof(T) + 3) & ~3)
#define va_start(ap, lastarg) (ap = ((char*)&lastarg + va_rsize(lastarg)))
#define va_end(ap)
#define va_arg(ap, T) (ap += va_rsize(T), *((T*)(ap - va_rsize(T))))

#else
# error "unknown arch"
#endif

#endif
