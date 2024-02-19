#include "token-stream.h"
#include <allocators/fmalloc.h>
#include <containers/array.h>
#include <containers/string.h>
#include <stdlib.h>

char const * token_strings[el_token_type_count] = {
	"_None",          // el_NONE

	"_EndLine",       // el_END_LINE

	"_LineComment",   // el_LINE_COMMENT

	"_Identifier",    // el_IDENTIFIER

	"_NumberLiteral", // el_NUMBER_LITERAL
	"_StringLiteral", // el_STRING_LITERAL

	"int",            // el_INT_TYPE
	"float",          // el_FLOAT_TYPE

	"fn",             // el_FNC_KEYWORD
	"struct",         // el_DAT_KEYWORD
	"let",            // el_LET_KEYWORD
	"return",         // el_RET_KEYWORD
	"for",            // el_FOR_KEYWORD
	"in",             // el_IN_KEYWORD
	"if",             // el_IF_KEYWORD
	"elif",           // el_ELIF_KEYWORD
	"else",           // el_ELSE_KEYWORD

	"{",              // el_BLOCK_START
	"}",              // el_BLOCK_END

	"(",              // el_PARENTHESIS_OPEN
	")",              // el_PARENTHESIS_CLOSE

	"[",              // el_LIST_START
	"]",              // el_LIST_END

	"=",              // el_ASSIGN_OPERATOR
	".",              // el_DOT_OPERATOR
	",",              // el_COMMA_SEPARATOR

	"||",             // el_BOOLEAN_OR
	"&&",             // el_BOOLEAN_AND

	"+",              // el_PLUS_OPERATOR
	"-",              // el_MINUS_OPERATOR
	"*",              // el_MULTIPLY_OPERATOR
	"/",              // el_DIVIDE_OPERATOR

	"==",             // el_EQUALS_COMPARATOR
	"<",              // el_LESS_THAN_COMPARATOR
	">",              // el_GREATER_THAN_COMPARATOR	
	"<=",             // el_LEQUALS_COMPARATOR
	">=",             // el_GEQUALS_COMPARATOR

	"->"              // el_RETURNS_OPERATOR
};

static_assert(ARRAY_SIZE(token_strings) == el_token_type_count, "Lexer's token_strings array is not up-to-date with el_token_type");

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
