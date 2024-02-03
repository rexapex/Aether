#pragma once
#include <containers/string.h>

struct el_text_file
{
	el_string contents;
	el_string path;
};

// Open a file and load its contents into an el_text_file object
// The file must be deleted with el_text_file_delete
struct el_text_file el_text_file_new(char const * path);

// Delete an el_text_file_delete
// Must be called to free internal memory
void el_text_file_delete(struct el_text_file * f);
