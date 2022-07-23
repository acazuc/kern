#ifndef STDARG_H
#define STDARG_H

typedef __builtin_va_list va_list;

#define va_start(ap, lastarg) __builtin_va_start(ap, lastarg)
#define va_end(ap) __builtin_va_end(ap)
#define va_arg(ap, T) __builtin_va_arg(ap, T)

#endif
