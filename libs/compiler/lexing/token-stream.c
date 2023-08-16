#include "token-stream.h"

void el_token_stream_delete(struct el_token_stream * stream)
{
	if(stream)
	{
		for(int i = 0; i < stream->num_tokens; ++i)
		{
			ffree(stream->tokens[i].source);
		}

		ffree(stream->tokens);
		stream->tokens = NULL;
	}
}
