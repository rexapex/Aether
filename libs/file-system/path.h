#pragma once
#include <assert.h>
#include <containers/string.h>
#include <allocators/fmalloc.h>

inline el_string el_combine_paths(el_string * p1, el_string * p2)
{
	return NULL;
//	assert(p1 && p2);
//	el_string new_path;
//#ifdef SYSTEM_WINDOWS
//	new_path.length = p1->length + p2->length + 1;
//	new_path.data = fmalloc(new_path.length);
//#elif defined(SYSTEM_LINUX)
//	// TODO
//#endif
}

inline el_string el_filename(el_string * path)
{
	return NULL;
	//assert(path && path->data);
	//el_string filename;
	//return filename;
	// TODO - Extract by finding last \ or / and getting what comes after
	// loop from end to start checking each char for \ or /
}
