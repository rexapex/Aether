#pragma once
#include <stdlib.h>

static inline void * fmalloc(size_t size)
{
	return malloc(size);
}

static inline void ffree(void * ptr)
{
	free(ptr);
}
