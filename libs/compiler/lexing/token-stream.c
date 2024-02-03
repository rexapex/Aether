#include "token-stream.h"
#include <allocators/fmalloc.h>
#include <containers/string.h>
#include <stdlib.h>

void el_token_stream_delete(struct el_token_stream * stream)
{
	if(stream)
	{
		stream->current_token = 0;

		for(int i = 0; i < stream->num_tokens; ++i)
		{
			el_string_delete(stream->tokens[i].source);
		}
		stream->num_tokens = 0;

		ffree(stream->tokens);
		stream->tokens = NULL;
	}
}
