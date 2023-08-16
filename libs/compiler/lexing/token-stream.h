#pragma once
#include <containers/string.h>

enum el_token_type
{
	el_NONE = 0,

	el_END_LINE,

	el_LINE_COMMENT,

	el_IDENTIFIER,

	el_NUMBER_LITERAL,
	el_STRING_LITERAL,

	el_INT_TYPE,
	el_FLOAT_TYPE,

	el_FNC_KEYWORD,
	el_DAT_KEYWORD,
	el_RET_KEYWORD,
	el_FOR_KEYWORD,
	el_IF_KEYWORD,
	el_ELIF_KEYWORD,
	el_ELSE_KEYWORD,

	el_BLOCK_START,
	el_BLOCK_END,

	el_PARAM_LIST_START,
	el_PARAM_LIST_END,

	el_SLICE_START,
	el_SLICE_END,

	el_ASSIGN_OPERATOR,
	el_DOT_OPERATOR,
	el_COMMA_SEPARATOR,

	el_PLUS_OPERATOR,
	el_MINUS_OPERATOR,
	el_MULTIPLY_OPERATOR,
	el_DIVIDE_OPERATOR,

	el_token_type_count
};

struct el_token
{
	enum el_token_type type;
	el_string source;
};

struct el_token_stream
{
	struct el_token * tokens;
	int num_tokens;
};

void el_token_stream_delete(struct el_token_stream * stream);
