#ifndef INTTYPES_H
#define INTTYPES_H

#if defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)

#define PRIi8 "i"
#define PRId8 "d"
#define PRIu8 "u"
#define PRIo8 "o"
#define PRIx8 "x"
#define PRIX8 "X"

#define PRIi16 "i"
#define PRId16 "d"
#define PRIu16 "u"
#define PRIo16 "o"
#define PRIx16 "x"
#define PRIX16 "X"

#define PRIi32 "li"
#define PRId32 "ld"
#define PRIu32 "lu"
#define PRIo32 "lo"
#define PRIx32 "lx"
#define PRIX32 "lX"

#define PRIi64 "lli"
#define PRId64 "lld"
#define PRIu64 "llu"
#define PRIo64 "llo"
#define PRIx64 "llx"
#define PRIX64 "llX"

#else
# error "unknown arch"
#endif

#endif
