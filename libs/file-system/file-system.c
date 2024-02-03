#include "file-system.h"
#include <allocators/fmalloc.h>
#include <containers/string.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

struct el_text_file el_text_file_new(char const * path)
{
	struct el_text_file f;
	f.contents = NULL;
	f.path = el_string_new(path, -1);

	FILE * fptr = fopen(path, "r");

	if(!fptr)
	{
		fprintf(stderr, "Failed to open file: %s, error %d %s\n", path, errno, strerror(errno));
		return f;
	}

	if(fseek(fptr, 0, SEEK_END) == 0)
	{
		int length = (int)ftell(fptr);
		if(length < 0)
		{
			goto close_file;
		}

		f.contents = el_string_new(NULL, length);

		// Seek back to start of file
		if(fseek(fptr, 0, SEEK_SET) != 0)
		{
			goto close_file;
		}

		// Read the file into memory
		int actual_length = (int)fread(f.contents, sizeof(char), length, fptr);
		int err = ferror(fptr);
		if(err != 0 || actual_length < 0)
		{
			fprintf(stderr, "Failed to read file: %s, read length %d, error %d %s\n", path, actual_length, errno, strerror(errno));
			goto close_file;
		}

		el_string_shrink(f.contents, actual_length);
		f.contents[actual_length] = '\0';
	}

close_file:
	fclose(fptr);

	return f;
}

void el_text_file_delete(struct el_text_file * f)
{
	if(f)
	{
		el_string_delete(f->contents);
		el_string_delete(f->path);
		f->contents = NULL;
		f->path = NULL;
	}
}
