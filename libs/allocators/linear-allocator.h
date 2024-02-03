#pragma once
#include <stdbool.h>
#include <uchar.h>

struct el_linear_allocator
{
	unsigned char * memory;
	size_t capacity;
	size_t size;
};

void * el_linear_alloc(struct el_linear_allocator * allocator, size_t num_bytes);

void el_linear_allocator_reset(struct el_linear_allocator * allocator, bool zero_memory);
