#pragma once
#include <stdlib.h>

inline void * fmalloc(size_t size)
{
	return malloc(size);
}

inline void ffree(void * ptr)
{
	free(ptr);
}
