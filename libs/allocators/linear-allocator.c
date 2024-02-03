#include "linear-allocator.h"
#include <string.h>

void * el_linear_alloc(struct el_linear_allocator * allocator, size_t num_bytes)
{
	if(!allocator || !allocator->memory)
	{
		return NULL;
	}

	int remaining_capacity = allocator->capacity - allocator->size;
	if(remaining_capacity < num_bytes)
	{
		return NULL;
	}

	unsigned char * ptr = allocator->memory + allocator->size;
	allocator->size += num_bytes;
	return ptr;
}

void el_linear_allocator_reset(struct el_linear_allocator * allocator, bool zero_memory)
{
	if(!allocator || !allocator->memory)
	{
		return;
	}

	if(zero_memory)
	{
		memset(allocator->memory, 0, allocator->size);
	}

	allocator->size = 0;
}
