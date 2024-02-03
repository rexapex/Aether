#pragma once
#include <stdbool.h>

// el_string is a wrapper around a char *, with an associated length property
// length does not include the ending null character
// Can be used in all places where a regular char * can be used
typedef char * el_string;

el_string el_string_new(char const * c, int length);

// Create a new string in the pre-allocated dst pointer
// dst pointer must have size of at least (length + (sizeof length) + 1)
el_string el_string_inplace_new(char * restrict dst, int dst_size, char const * restrict c, int length);

// Delete a string created by el_string_new
// Do not call this if string was created using el_string_inplace_new
void el_string_delete(el_string s);

int el_string_length(el_string s);

// Returns the number of bytes required to store the string
// The underlying memory allocated for the string is at least this large
int el_string_byte_size(el_string s);

bool el_string_equals(el_string s1, el_string s2);

void el_string_shrink(el_string s, int length);

el_string el_string_strip(el_string s);
