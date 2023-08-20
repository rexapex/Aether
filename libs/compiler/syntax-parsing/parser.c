#include "parser.h"
#include <compiler/lexing/token-stream.h>
#include <stdio.h>
#include <stdbool.h>

#if 0
	#define DEBUG_PRODUCTION(x) printf(x); printf("\n");
#else
	#define DEBUG_PRODUCTION(x)
#endif

static int el_parse_new_line(struct el_token_stream * token_stream);
static int el_parse_new_lines(struct el_token_stream * token_stream);
static int el_parse_statements(struct el_token_stream * token_stream);
static int el_parse_statement(struct el_token_stream * token_stream);

static int el_parse_function(struct el_token_stream * token_stream);
static int el_parse_parameter_list(struct el_token_stream * token_stream);
static int el_parse_parameters(struct el_token_stream * token_stream);
static int el_parse_parameter(struct el_token_stream * token_stream);

static int el_parse_code_block(struct el_token_stream * token_stream);
static int el_parse_code_block_statements(struct el_token_stream * token_stream);
static int el_parse_code_block_statement(struct el_token_stream * token_stream);

static int el_parse_for_statement(struct el_token_stream * token_stream);
static int el_parse_if_statement(struct el_token_stream * token_stream);
static int el_parse_elif_statements(struct el_token_stream * token_stream);
static int el_parse_else_statement(struct el_token_stream * token_stream);

static int el_parse_assignment(struct el_token_stream * token_stream);
static int el_parse_expr(struct el_token_stream * token_stream);
static int el_parse_function_call(struct el_token_stream * token_stream);
static int el_parse_arguments(struct el_token_stream * token_stream);
static int el_parse_argument(struct el_token_stream * token_stream);

static int el_parse_data_block(struct el_token_stream * token_stream);
static int el_parse_data_block_statements(struct el_token_stream * token_stream);
static int el_parse_data_block_statement(struct el_token_stream * token_stream);

static int el_parse_optional_type(struct el_token_stream * token_stream);
static int el_parse_type(struct el_token_stream * token_stream);

static int el_parse_complex_identifier(struct el_token_stream * token_stream);
static int el_parse_complex_identifier_inner(struct el_token_stream * token_stream);

static int el_match_token(struct el_token_stream * token_stream, int type);
static bool el_is_lookahead(struct el_token_stream * token_stream, int type);

struct el_ast el_parse_token_stream(struct el_token_stream * token_stream)
{
	struct el_ast ast = {
		.root = fmalloc(sizeof(struct el_ast_node))
	};

	ast.root->token.type = el_NONE;
	ast.root->token.source = NULL;

	if(el_parse_statements(token_stream) != 0)
	{
		fprintf(stderr, "Failed to parse token stream\n");
	}

	return ast;
}

// NOTE - In the production functions below the pattern err = err || ... is used
// Do NOT change to err |= as short circuiting is desired

static int el_parse_new_line(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_new_line");
	return el_match_token(token_stream, el_END_LINE);
}

static int el_parse_new_lines(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_new_lines");
	int err = 0;
	while(el_is_lookahead(token_stream, el_END_LINE))
	{
		err = err || el_parse_new_line(token_stream);
	}
	return err;
}

static int el_parse_statements(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_statements");
	int err = 0;
	if(token_stream->current_token < token_stream->num_tokens)
	{
		// Parse a single statement
		err = err || el_parse_new_lines(token_stream);
		err = err || el_parse_statement(token_stream);
		err = err || el_parse_new_lines(token_stream);

		// Parse additional statements by recursing into this fn
		err = err || el_parse_statements(token_stream);

	}
	return err;
}

static int el_parse_statement(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_statement");
	int err = 0;
	if(el_is_lookahead(token_stream, el_FNC_KEYWORD))
	{
		err = err || el_parse_function(token_stream);
	}
	else if(el_is_lookahead(token_stream, el_DAT_KEYWORD))
	{
		err = err || el_parse_data_block(token_stream);
	}
	else
	{
		// Statements which are valid within code blocks are also valid at file scope
		err = err || el_parse_code_block_statement(token_stream);
	}
	return err;
}

static int el_parse_function(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_function");
	int err = 0;
	err = err || el_match_token(token_stream, el_FNC_KEYWORD);
	err = err || el_match_token(token_stream, el_IDENTIFIER);
	err = err || el_parse_parameter_list(token_stream);
	err = err || el_parse_optional_type(token_stream);
	err = err || el_parse_code_block(token_stream);
	return err;
}

static int el_parse_parameter_list(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_parameter_list");
	int err = 0;
	err = err || el_match_token(token_stream, el_PARENTHESIS_OPEN);
	err = err || el_parse_parameters(token_stream);
	err = err || el_match_token(token_stream, el_PARENTHESIS_CLOSE);
	return err;
}

static int el_parse_parameters(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_parameters");
	int err = 0;
	if(el_is_lookahead(token_stream, el_PARENTHESIS_CLOSE))
	{
		return err;
	}
	err = err || el_parse_parameter(token_stream);
	if(el_is_lookahead(token_stream, el_COMMA_SEPARATOR))
	{
		// NOTE - This allows a trailing comma before the closing bracket
		err = err || el_match_token(token_stream, el_COMMA_SEPARATOR);
		err = err || el_parse_parameters(token_stream);
	}
	return err;
}

static int el_parse_parameter(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_parameter");
	int err = 0;
	err = err || el_match_token(token_stream, el_IDENTIFIER);
	err = err || el_parse_optional_type(token_stream);
	return err;
}

static int el_parse_code_block(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_code_block");
	int err = 0;
	err = err || el_match_token(token_stream, el_BLOCK_START);
	err = err || el_parse_code_block_statements(token_stream);
	err = err || el_match_token(token_stream, el_BLOCK_END);
	err = err || el_parse_new_line(token_stream);
	return err;
}

static int el_parse_code_block_statements(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_code_block_statements");
	int err = 0;
	err = err || el_parse_new_lines(token_stream);
	if(el_is_lookahead(token_stream, el_BLOCK_END))
	{
		return err;
	}
	// Parse a single statement
	err = err || el_parse_code_block_statement(token_stream);
	// Parse additional statements by recursing into this fn
	err = err || el_parse_code_block_statements(token_stream);
	return err;
}

static int el_parse_code_block_statement(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_code_block_statement");
	int err = 0;
	if(el_is_lookahead(token_stream, el_FOR_KEYWORD))
	{
		err = err || el_parse_for_statement(token_stream);
	}
	else if(el_is_lookahead(token_stream, el_IF_KEYWORD))
	{
		err = err || el_parse_if_statement(token_stream);
	}
	else if(el_is_lookahead(token_stream, el_RET_KEYWORD))
	{
		err = err || el_match_token(token_stream, el_RET_KEYWORD);
		err = err || el_parse_expr(token_stream);
	}
	else
	{
		err = err || el_parse_complex_identifier(token_stream);
		if(el_is_lookahead(token_stream, el_ASSIGN_OPERATOR))
		{
			err = err || el_parse_assignment(token_stream);
		}
	}
	return err;
}

static int el_parse_for_statement(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_for_statement");
	int err = 0;
	err = err || el_match_token(token_stream, el_FOR_KEYWORD);
	err = err || el_match_token(token_stream, el_IDENTIFIER); // Index variable
	err = err || el_match_token(token_stream, el_COMMA_SEPARATOR);
	err = err || el_match_token(token_stream, el_IDENTIFIER); // Element variable
	err = err || el_match_token(token_stream, el_IN_KEYWORD);
	err = err || el_parse_complex_identifier(token_stream);
	err = err || el_parse_code_block(token_stream);
	return err;
}

static int el_parse_if_statement(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_if_statement");
	int err = 0;
	err = err || el_match_token(token_stream, el_IF_KEYWORD);
	err = err || el_parse_expr(token_stream);
	err = err || el_parse_code_block(token_stream);
	if(el_is_lookahead(token_stream, el_ELIF_KEYWORD))
	{
		err = err || el_parse_elif_statements(token_stream);
	}
	if(el_is_lookahead(token_stream, el_ELSE_KEYWORD))
	{
		err = err || el_parse_else_statement(token_stream);
	}
	return err;
}

static int el_parse_elif_statements(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_elif_statements");
	int err = 0;
	err = err || el_match_token(token_stream, el_ELIF_KEYWORD);
	err = err || el_parse_expr(token_stream);
	err = err || el_parse_code_block(token_stream);
	if(el_is_lookahead(token_stream, el_ELIF_KEYWORD))
	{
		err = err || el_parse_elif_statements(token_stream);
	}
	return err;
}

static int el_parse_else_statement(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_else_statement");
	int err = 0;
	err = err || el_match_token(token_stream, el_ELSE_KEYWORD);
	err = err || el_parse_code_block(token_stream);
	return err;
}

static int el_parse_assignment(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_assignment");
	int err = 0;
	err = err || el_match_token(token_stream, el_ASSIGN_OPERATOR);
	err = err || el_parse_expr(token_stream);
	return err;
}

static int el_parse_expr(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_expr");
	int err = 0;
	if(el_is_lookahead(token_stream, el_NUMBER_LITERAL))
	{
		err = err || el_match_token(token_stream, el_NUMBER_LITERAL);
	}
	else if(el_is_lookahead(token_stream, el_STRING_LITERAL))
	{
		err = err || el_match_token(token_stream, el_STRING_LITERAL);
	}
	else if(el_is_lookahead(token_stream, el_IDENTIFIER))
	{
		err = err || el_parse_complex_identifier(token_stream);
	}
	else if(el_is_lookahead(token_stream, el_PARENTHESIS_OPEN))
	{
		err = err || el_match_token(token_stream, el_PARENTHESIS_OPEN);
		err = err || el_parse_expr(token_stream);
		err = err || el_match_token(token_stream, el_PARENTHESIS_CLOSE);
	}
	else if(el_is_lookahead(token_stream, el_SLICE_START))
	{
		err = err || el_match_token(token_stream, el_SLICE_START);
		err = err || el_parse_arguments(token_stream);
		err = err || el_match_token(token_stream, el_SLICE_END);
	}
	else
	{
		fprintf(stderr, "Expected an expression\n");
		err = 1;
	}

	if(el_is_lookahead(token_stream, el_ADD_OPERATOR))
	{
		err = err || el_match_token(token_stream, el_ADD_OPERATOR);
		err = err || el_parse_expr(token_stream);
	}
	else if(el_is_lookahead(token_stream, el_SUBTRACT_OPERATOR))
	{
		err = err || el_match_token(token_stream, el_SUBTRACT_OPERATOR);
		err = err || el_parse_expr(token_stream);
	}
	else if(el_is_lookahead(token_stream, el_MULTIPLY_OPERATOR))
	{
		err = err || el_match_token(token_stream, el_MULTIPLY_OPERATOR);
		err = err || el_parse_expr(token_stream);
	}
	else if(el_is_lookahead(token_stream, el_DIVIDE_OPERATOR))
	{
		err = err || el_match_token(token_stream, el_DIVIDE_OPERATOR);
		err = err || el_parse_expr(token_stream);
	}
	else if(el_is_lookahead(token_stream, el_EQUALS_COMPARATOR))
	{
		err = err || el_match_token(token_stream, el_EQUALS_COMPARATOR);
		err = err || el_parse_expr(token_stream);
	}
	else if(el_is_lookahead(token_stream, el_GREATER_THAN_COMPARATOR))
	{
		err = err || el_match_token(token_stream, el_GREATER_THAN_COMPARATOR);
		err = err || el_parse_expr(token_stream);
	}
	else if(el_is_lookahead(token_stream, el_LESS_THAN_COMPARATOR))
	{
		err = err || el_match_token(token_stream, el_LESS_THAN_COMPARATOR);
		err = err || el_parse_expr(token_stream);
	}
	else if(el_is_lookahead(token_stream, el_LEQUALS_COMPARATOR))
	{
		err = err || el_match_token(token_stream, el_LEQUALS_COMPARATOR);
		err = err || el_parse_expr(token_stream);
	}
	else if(el_is_lookahead(token_stream, el_GEQUALS_COMPARATOR))
	{
		err = err || el_match_token(token_stream, el_GEQUALS_COMPARATOR);
		err = err || el_parse_expr(token_stream);
	}

	return err;
}

static int el_parse_function_call(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_function_call");
	int err = 0;
	err = err || el_match_token(token_stream, el_PARENTHESIS_OPEN);
	err = err || el_parse_arguments(token_stream);
	err = err || el_match_token(token_stream, el_PARENTHESIS_CLOSE);
	return err;
}

static int el_parse_arguments(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_arguments");
	int err = 0;
	if(el_is_lookahead(token_stream, el_PARENTHESIS_CLOSE))
	{
		return err;
	}
	err = err || el_parse_argument(token_stream);
	if(el_is_lookahead(token_stream, el_COMMA_SEPARATOR))
	{
		// NOTE - This allows a trailing comma before the closing bracket
		err = err || el_match_token(token_stream, el_COMMA_SEPARATOR);
		err = err || el_parse_arguments(token_stream);
	}
	return err;
}

static int el_parse_argument(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_argument");
	int err = 0;
	err = err || el_parse_expr(token_stream);
	return err;
}

static int el_parse_data_block(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_data_block");
	int err = 0;
	err = err || el_match_token(token_stream, el_DAT_KEYWORD);
	err = err || el_match_token(token_stream, el_IDENTIFIER);
	err = err || el_match_token(token_stream, el_BLOCK_START);
	err = err || el_parse_data_block_statements(token_stream);
	err = err || el_match_token(token_stream, el_BLOCK_END);
	err = err || el_parse_new_line(token_stream);
	return err;
}

static int el_parse_data_block_statements(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_data_block_statements");
	int err = 0;
	err = err || el_parse_new_lines(token_stream);
	if(el_is_lookahead(token_stream, el_BLOCK_END))
	{
		return err;
	}
	// Parse a single statement
	err = err || el_parse_data_block_statement(token_stream);
	// Parse additional statements by recursing into this fn
	err = err || el_parse_data_block_statements(token_stream);
	return err;
}

static int el_parse_data_block_statement(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_data_block_statement");
	int err = 0;
	err = err || el_match_token(token_stream, el_IDENTIFIER);
	err = err || el_parse_type(token_stream);
	return err;
}

static int el_parse_optional_type(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_optional_type");
	int err = 0;
	if(
		el_is_lookahead(token_stream, el_IDENTIFIER) ||
		el_is_lookahead(token_stream, el_INT_TYPE) ||
		el_is_lookahead(token_stream, el_FLOAT_TYPE))
	{
		err = err || el_parse_type(token_stream);
	}
	return err;
}

static int el_parse_type(struct el_token_stream * token_stream)
{
	DEBUG_PRODUCTION("el_parse_type");
	int err = 0;
	if(el_is_lookahead(token_stream, el_IDENTIFIER))
	{
		err = err || el_match_token(token_stream, el_IDENTIFIER);
	}
	else if(el_is_lookahead(token_stream, el_INT_TYPE))
	{
		err = err || el_match_token(token_stream, el_INT_TYPE);
	}
	else if(el_is_lookahead(token_stream, el_FLOAT_TYPE))
	{
		err = err || el_match_token(token_stream, el_FLOAT_TYPE);
	}
	else
	{
		fprintf(stderr, "Expected a type, got %d %s\n", token_stream->tokens[token_stream->current_token].type, token_stream->tokens[token_stream->current_token].source);
		err = 1;
	}
	
	// Parse multiple slice open & close tokens to support multi-dimensional slices
	while(el_is_lookahead(token_stream, el_SLICE_START) && err == 0)
	{
		err = err || el_match_token(token_stream, el_SLICE_START);
		err = err || el_match_token(token_stream, el_SLICE_END);
	}
	return err;
}

static int el_parse_complex_identifier(struct el_token_stream * token_stream)
{
	int err = 0;
	err = err || el_match_token(token_stream, el_IDENTIFIER);
	err = err || el_parse_complex_identifier_inner(token_stream);
	return err;
}

static int el_parse_complex_identifier_inner(struct el_token_stream * token_stream)
{
	int err = 0;
	if(el_is_lookahead(token_stream, el_SLICE_START))
	{
		err = err || el_match_token(token_stream, el_SLICE_START);
		err = err || el_parse_expr(token_stream);
		err = err || el_match_token(token_stream, el_SLICE_END);
	}
	else if(el_is_lookahead(token_stream, el_PARENTHESIS_OPEN))
	{
		err = err || el_parse_function_call(token_stream);
	}
	else if(el_is_lookahead(token_stream, el_DOT_OPERATOR))
	{
		err = err || el_match_token(token_stream, el_DOT_OPERATOR);
		err = err || el_match_token(token_stream, el_IDENTIFIER);
	}
	else
	{
		return err;
	}
	err = err || el_parse_complex_identifier_inner(token_stream);
	return err;
}

// Match the current lookahead token to the type given
// If the types do not match, a non-zero error code is returned
static int el_match_token(struct el_token_stream * token_stream, int type)
{
	int lookahead = token_stream->tokens[token_stream->current_token].type;
	if(lookahead == type)
	{
	#if 1
		printf("Matched token: %d %s\n", type, token_stream->tokens[token_stream->current_token].source);
	#endif
		++token_stream->current_token;
		return 0;
	}

	fprintf(stderr, "Failed to match token at lookahead index %d: expected %d, got %d\n", token_stream->current_token, type, lookahead);
	return 1;
}

static bool el_is_lookahead(struct el_token_stream * token_stream, int type)
{
	if(token_stream->current_token >= token_stream->num_tokens)
	{
		printf("Failed to check lookahead type, ran out of tokens");
		return false;
	}

	return token_stream->tokens[token_stream->current_token].type == type;
}
