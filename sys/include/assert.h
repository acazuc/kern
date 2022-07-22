#ifndef ASSERT_H
#define ASSERT_H

#define assert(expression, ...) \
do \
{ \
	if (!(expression)) \
		panic(__VA_ARGS__); \
} while (0)

#endif
