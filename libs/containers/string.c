#include "string.h"
#include <allocators/fmalloc.h>
#include <assert.h>
#include <string.h>

el_string el_string_new(char const * c, int length)
{
	if(length < 0)
	{
		length = (int)strlen(c);
	}

	char * s = fmalloc(length + (sizeof length) + 1);
	if(!s)
	{
		return NULL;
	}

	*(int *)s = length;
	char * contents = s + sizeof length;

	if(c != NULL)
	{
		memcpy(contents, c, (size_t)length + 1);
	}

	contents[length] = '\0';
	return contents;
}

el_string el_string_inplace_new(char * restrict dst, int dst_size, char const * restrict c, int length)
{
	if(!dst)
	{
		return NULL;
	}

	if(length < 0)
	{
		length = (int)strlen(c);
	}

	if(dst_size < length + (sizeof length) + 1)
	{
		return NULL;
	}

	*(int *)dst = length;
	char * contents = dst + sizeof length;

	if(c != NULL)
	{
		memcpy(contents, c, (size_t)length + 1);
	}

	contents[length] = '\0';
	return contents;
}

void el_string_delete(el_string s)
{
	if(s)
	{
		ffree(s - sizeof(int));
	}
}

int el_string_length(el_string s)
{
	assert(s);
	return *(int *)(s - sizeof(int));
}

int el_string_byte_size(el_string s)
{
	assert(s);
	// If size is >= maxint - sizeof(int) then this is undefined behaviour as byte size will not fit in int
	return *(int *)(s - sizeof(int)) + sizeof(int) + 1;
}

bool el_string_equals(el_string s1, el_string s2)
{
	assert(s1 && s2);
	return strcmp(s1, s2) == 0;
}

void el_string_shrink(el_string s, int length)
{
	assert(s);
	if(length >= 0 && length < el_string_length(s))
	{
		*(int *)(s - sizeof length) = length;
		s[length] = '\0';
	}
}

el_string el_string_strip(el_string s)
{
	assert(s);
	char const * start = s;
	char const * end = start + el_string_length(s) - 1;

	// Find first non-whitespace character
	for(; start < end; ++start)
	{
		if(*start != ' ' && *start != '\t')
		{
			break;
		}
	}

	// Find last non-whitespace character
	for(; end > start; --end)
	{
		if(*end != ' ' && *end != '\t')
		{
			break;
		}
	}

	return el_string_new(start, (int)(end - start + 1));
}
