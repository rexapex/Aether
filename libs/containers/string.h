#pragma once
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <allocators/fmalloc.h>

// el_string is a wrapper around a char *, with an associated length property
// length does not include the ending null character
// can be used in all places where a regular char * can be used
typedef char * el_string;

inline el_string el_string_new(char const * c, int length)
{
	if(length < 0) {
		length = (int)strlen(c);
	}

	char * s = fmalloc(length + sizeof length + 1);
	if(!s) {
		return NULL;
	}

	*s = length;
	char * contents = s + sizeof length;

	if(c != NULL) {
		memcpy(contents, c, (size_t)length + 1);
	}

	contents[length] = '\0';
	return contents;
}

inline void el_string_delete(el_string s)
{
	assert(s);
	ffree(s - sizeof(int));
}

inline int el_string_length(el_string s)
{
	assert(s);
	return *(s - sizeof(int));
}

inline bool el_string_equals(el_string s1, el_string s2)
{
	assert(s1 && s2);
	return strcmp(s1, s2) == 0;
}

inline void el_string_shrink(el_string s, int length)
{
	assert(s);
	if(length <= el_string_length(s)) {
		*(s - sizeof length) = length;
	}
}

inline el_string el_string_strip(el_string s)
{
	assert(s);
	char * start = s;
	char * end = start + el_string_length(s) - 1;

	// Find first non-whitespace character
	for(; start < end; ++start) {
		if(*start != ' ' && *start != '\t') {
			break;
		}
	}

	// Find last non-whitespace character
	for(; end > start; --end) {
		if(*end != ' ' && *end != '\t') {
			break;
		}
	}

	return el_string_new(start, (int)(end - start + 1));
}
