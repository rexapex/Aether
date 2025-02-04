#include "lexer.h"
#include <file-system/file-system.h>
#include <allocators/fmalloc.h>
#include <containers/array.h>
#include <containers/string.h>
#include <compiler/error.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#ifdef SYSTEM_WINDOWS
	#define strtok_r strtok_s
#endif

#define DEBUG_LEXING 0

#define MAX_TOKEN_LENGTH 128
#define MAX_NUM_TOKENS_PER_FILE 4096

static char const * token_strings[] = {
	"N/A",		// el_NONE

	"N/A",		// el_END_LINE

	"N/A",		// el_LINE_COMMENT

	"N/A",		// el_IDENTIFIER

	"N/A",		// el_NUMBER_LITERAL
	"N/A",		// el_STRING_LITERAL

	"int",		// el_INT_TYPE
	"float",	// el_FLOAT_TYPE

	"fnc",		// el_FNC_KEYWORD
	"dat",		// el_DAT_KEYWORD
	"ret",		// el_RET_KEYWORD
	"for",		// el_FOR_KEYWORD
	"in",		// el_IN_KEYWORD
	"if",		// el_IF_KEYWORD
	"elif",		// el_ELIF_KEYWORD
	"else",		// el_ELSE_KEYWORD

	"{",		// el_BLOCK_START
	"}",		// el_BLOCK_END

	"(",		// el_PARENTHESIS_OPEN
	")",		// el_PARENTHESIS_CLOSE

	"[",		// el_SLICE_START
	"]",		// el_SLICE_END

	"=",		// el_ASSIGN_OPERATOR
	".",		// el_DOT_OPERATOR
	",",		// el_COMMA_SEPARATOR

	"or",		// el_BOOLEAN_OR
	"and",		// el_BOOLEAN_AND

	"+",		// el_PLUS_OPERATOR
	"-",		// el_MINUS_OPERATOR
	"*",		// el_MULTIPLY_OPERATOR
	"/",		// el_DIVIDE_OPERATOR

	"==",		// el_EQUALS_COMPARATOR
	"<",		// el_LESS_THAN_COMPARATOR
	">",		// el_GREATER_THAN_COMPARATOR	
	"<=",		// el_LEQUALS_COMPARATOR
	">="		// el_GEQUALS_COMPARATOR
};

static_assert(ARRAY_SIZE(token_strings) == el_token_type_count, "Lexer's token_strings array is not up-to-date with el_token_type");

static int el_push_token(struct el_token_stream * stream, int type, el_string source)
{
	if(stream->num_tokens >= MAX_NUM_TOKENS_PER_FILE)
	{
		fprintf(stderr, "Failed to push token, %d, reached max number of tokens\n", type);
		return el_EXCEEDED_TOKENS_LIMIT_LEX_ERROR;
	}

	struct el_token token = {
		.type = type,
		.source = source
	};
	stream->tokens[stream->num_tokens++] = token;
	return el_SUCCESS;
}

static int el_lex_line(char const * line, struct el_token_stream * stream)
{
	bool forming_string = false;

	char token_buf[MAX_TOKEN_LENGTH];
	int token_idx = 0;
	token_buf[0] = '\0';

	int num_tokens = 0;

	int length = (int)strlen(line);
	int start_i = 0;
	for(int i = 0; i < length + 1; ++i)
	{
		char c = (i < length ? line[i] : '\n');

		if(token_idx >= MAX_TOKEN_LENGTH)
		{
			fprintf(stderr, "Failed to lex line, token greater than max length: %d\n", MAX_TOKEN_LENGTH);
			return el_EXCEEDED_TOKEN_LENGTH_LIMIT_LEX_ERROR;
		}

		// If the character is a delimiter then the token currently being built is complete
		if(!forming_string
			&& (c == ' ' || c == '\n' || c == '\r' || c == '\t'
			|| c == '}' || c == '{' || c == '(' || c == ')'
			|| c == '[' || c == ']' || c == '.' || c == ','))
		{
			int token_type = el_NONE;
			int delim_type = el_NONE;

			for(int j = 0; j < ARRAY_SIZE(token_strings); ++j)
			{
				// Get the type of the token being built
				if(strcmp(token_buf, token_strings[j]) == 0)
				{
					token_type = j;
				}

				// Get the type of the delimiter
				if(strlen(token_strings[j]) == 1 && token_strings[j][0] == c)
				{
					delim_type = j;
				}
			}

			if(token_type == el_NONE)
			{
				// If the token starts with a digit, it is considered a number literal, else an identifier
				token_type = token_buf[0] >= 48 && token_buf[0] <= 57 ? el_NUMBER_LITERAL : el_IDENTIFIER;
			}

			// Ignore zero-length tokens
			if(token_idx > 0)
			{
			#if DEBUG_LEXING
				printf("Token: %s   %d\n", token_buf, token_type);
			#endif
				int err = el_push_token(stream, token_type, el_string_new(token_buf, -1));
				if(err != el_SUCCESS)
					return err;
			}

			// Not all delimiters form tokens (e.g. whitespace is ignored)
			if(delim_type != el_NONE)
			{
			#if DEBUG_LEXING
				printf("Token: %c   %d\n", c, delim_type);
			#endif
				int err = el_push_token(stream, delim_type, el_string_new(&c, 1));
				if(err != el_SUCCESS)
					return err;
			}

			// Reset token buffer ready to build next token
			token_buf[0] = '\0';
			token_idx = 0;
		}
		else if(c == '"')
		{
			if(forming_string)
			{
				forming_string = false;
			#if DEBUG_LEXING
				printf("String: %s\n", token_buf);
			#endif
				int err = el_push_token(stream, el_STRING_LITERAL, el_string_new(token_buf, -1));
				if(err != el_SUCCESS)
					return err;

				// Reset token buffer ready to build next token
				token_buf[0] = '\0';
				token_idx = 0;
			}
			else
			{
				forming_string = true;
			}
		}
		else if(c == '/' && i + 1 < length && line[i + 1] == '/')
		{
			// Line comment has started so don't lex the rest of this line
		#if DEBUG_LEXING
			printf("Started line comment, ignoring rest of line\n");
		#endif
			break;
		}
		else
		{
			token_buf[token_idx++] = c;
			token_buf[token_idx] = '\0';
		}
	}

	return el_SUCCESS;
}

struct el_token_stream el_lex_file(struct el_text_file * f)
{
	assert(f);
	struct el_token_stream stream = {
		.tokens = fmalloc(MAX_NUM_TOKENS_PER_FILE * sizeof(struct el_token)), // TODO - Change to variable size array
		.num_tokens = 0,
		.current_token = 0
	};

	if(!stream.tokens)
	{
		fprintf(stderr, "Failed to allocate tokens list\n");
		return stream;
	}

	char * next_line = NULL;
	char * line = strtok_r(f->contents, "\n", &next_line);
	while(line != NULL)
	{
	#if DEBUG_LEXING
		printf("Line: %s\n", line);
	#endif

		if(el_lex_line(line, &stream) != el_SUCCESS)
		{
			el_token_stream_delete(&stream);
			return stream;
		}

		if(el_push_token(&stream, el_END_LINE, el_string_new("\n", 1)) != el_SUCCESS)
		{
			el_token_stream_delete(&stream);
			return stream;
		}

		line = strtok_r(NULL, "\n", &next_line);
	}

	return stream;
}
