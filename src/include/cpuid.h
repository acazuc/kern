#ifndef CPUID_H
#define CPUID_H

#define __cpuid(n, eax, ebx, ecx, edx) __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(n))

#endif
