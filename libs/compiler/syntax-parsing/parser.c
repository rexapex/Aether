#include "parser.h"
#include <compiler/lexing/token-stream.h>
#include <stdio.h>
#include <stdbool.h>

#if 0
	#define DEBUG_PRODUCTION(x) printf(x); printf("\n");
#else
	#define DEBUG_PRODUCTION(x)
#endif

#define MAX_NUM_NODES_PER_NODE_LIST 128
#define MAX_NUM_ELIFS_PER_IF 8
#define MAX_NUM_VARS_PER_DATA_BLOCK 16

static int el_parse_new_line(struct el_token_stream * token_stream);
static int el_parse_new_lines(struct el_token_stream * token_stream);
static int el_parse_statements(struct el_token_stream * token_stream, struct el_ast_statement_list * list);
static int el_parse_statement(struct el_token_stream * token_stream, struct el_ast_statement_list * list);

static int el_parse_function(struct el_token_stream * token_stream, struct el_ast_statement_list * parent);
static int el_parse_parameter_list(struct el_token_stream * token_stream);
static int el_parse_parameters(struct el_token_stream * token_stream);
static int el_parse_parameter(struct el_token_stream * token_stream);

static int el_parse_code_block(struct el_token_stream * token_stream, struct el_ast_statement_list * list);
static int el_parse_code_block_statements(struct el_token_stream * token_stream, struct el_ast_statement_list * list);
static int el_parse_code_block_statement(struct el_token_stream * token_stream, struct el_ast_statement_list * list);

static int el_parse_for_statement(struct el_token_stream * token_stream, struct el_ast_statement_list * parent);
static int el_parse_if_statement(struct el_token_stream * token_stream, struct el_ast_statement_list * parent);
static int el_parse_elif_statements(struct el_token_stream * token_stream, struct el_ast_if_statement * parent);
static int el_parse_else_statement(struct el_token_stream * token_stream, struct el_ast_if_statement * parent);

static int el_parse_assignment(struct el_token_stream * token_stream, struct el_ast_statement_list * parent);
static int el_parse_expr(struct el_token_stream * token_stream);
static int el_parse_function_call(struct el_token_stream * token_stream);
static int el_parse_arguments(struct el_token_stream * token_stream);
static int el_parse_argument(struct el_token_stream * token_stream);

static int el_parse_data_block(struct el_token_stream * token_stream, struct el_ast_statement_list * parent);
static int el_parse_data_block_statements(struct el_token_stream * token_stream, struct el_ast_data_block * data_block);
static int el_parse_data_block_statement(struct el_token_stream * token_stream, struct el_ast_data_block * data_block);

static int el_parse_optional_type(struct el_token_stream * token_stream, struct el_ast_var_type * var_type);
static int el_parse_type(struct el_token_stream * token_stream, struct el_ast_var_type * var_type);

static int el_parse_complex_identifier(struct el_token_stream * token_stream);
static int el_parse_complex_identifier_inner(struct el_token_stream * token_stream);

static int el_match_token(struct el_token_stream * token_stream, int type);
static struct el_token * el_lookahead(struct el_token_stream * token_stream);
static bool el_is_lookahead(struct el_token_stream * token_stream, int type);

struct el_ast el_parse_token_stream(struct el_token_stream * token_stream)
{
	struct el_ast ast = {
		.root.statements = fmalloc(sizeof(struct el_ast_statement) * MAX_NUM_NODES_PER_NODE_LIST),
		.root.max_num_statements = MAX_NUM_NODES_PER_NODE_LIST,
		.root.num_statements = 0
	};

	if(el_parse_statements(token_stream, &ast.root) != 0)
	{
		fprintf(stderr, "Failed to parse token stream\n");
	}

	el_ast_print(&ast);

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

static int el_parse_statements(struct el_token_stream * token_stream, struct el_ast_statement_list * list)
{
	DEBUG_PRODUCTION("el_parse_statements");
	int err = 0;
	if(token_stream->current_token < token_stream->num_tokens)
	{
		// Parse a single statement
		err = err || el_parse_new_lines(token_stream);
		err = err || el_parse_statement(token_stream, list);
		err = err || el_parse_new_lines(token_stream);

		// Parse additional statements by recursing into this fn
		err = err || el_parse_statements(token_stream, list);
	}
	return err;
}

static int el_parse_statement(struct el_token_stream * token_stream, struct el_ast_statement_list * list)
{
	DEBUG_PRODUCTION("el_parse_statement");
	int err = 0;
	if(el_is_lookahead(token_stream, el_FNC_KEYWORD))
	{
		err = err || el_parse_function(token_stream, list);
	}
	else if(el_is_lookahead(token_stream, el_DAT_KEYWORD))
	{
		err = err || el_parse_data_block(token_stream, list);
	}
	else
	{
		// Statements which are valid within code blocks are also valid at file scope
		err = err || el_parse_code_block_statement(token_stream, list);
	}
	return err;
}

static int el_parse_function(struct el_token_stream * token_stream, struct el_ast_statement_list * parent)
{
	DEBUG_PRODUCTION("el_parse_function");
	int err = 0;
	parent->statements[parent->num_statements].type = el_AST_NODE_FUNCTION_DEFINITION;
	struct el_ast_function_definition * function_definition = &parent->statements[parent->num_statements++].function_definition;
	assert(parent->num_statements <= parent->max_num_statements);
	err = err || el_match_token(token_stream, el_FNC_KEYWORD);
	function_definition->name = el_string_new(el_lookahead(token_stream)->source, -1);
	err = err || el_match_token(token_stream, el_IDENTIFIER);
	err = err || el_parse_parameter_list(token_stream);
	err = err || el_parse_optional_type(token_stream, &function_definition->return_type);
	err = err || el_parse_code_block(token_stream, &function_definition->code_block);
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
	struct el_ast_var_type todo;
	err = err || el_parse_optional_type(token_stream, &todo);
	return err;
}

static int el_parse_code_block(struct el_token_stream * token_stream, struct el_ast_statement_list * list)
{
	DEBUG_PRODUCTION("el_parse_code_block");
	int err = 0;
	list->num_statements = 0;
	list->max_num_statements = MAX_NUM_NODES_PER_NODE_LIST;
	list->statements = fmalloc(sizeof(struct el_ast_statement) * list->max_num_statements);
	err = err || el_match_token(token_stream, el_BLOCK_START);
	err = err || el_parse_code_block_statements(token_stream, list);
	err = err || el_match_token(token_stream, el_BLOCK_END);
	err = err || el_parse_new_line(token_stream);
	return err;
}

static int el_parse_code_block_statements(struct el_token_stream * token_stream, struct el_ast_statement_list * list)
{
	DEBUG_PRODUCTION("el_parse_code_block_statements");
	int err = 0;
	err = err || el_parse_new_lines(token_stream);
	if(el_is_lookahead(token_stream, el_BLOCK_END))
	{
		return err;
	}
	// Parse a single statement
	err = err || el_parse_code_block_statement(token_stream, list);
	// Parse additional statements by recursing into this fn
	err = err || el_parse_code_block_statements(token_stream, list);
	return err;
}

static int el_parse_code_block_statement(struct el_token_stream * token_stream, struct el_ast_statement_list * list)
{
	DEBUG_PRODUCTION("el_parse_code_block_statement");
	int err = 0;
	if(el_is_lookahead(token_stream, el_FOR_KEYWORD))
	{
		err = err || el_parse_for_statement(token_stream, list);
	}
	else if(el_is_lookahead(token_stream, el_IF_KEYWORD))
	{
		err = err || el_parse_if_statement(token_stream, list);
	}
	else if(el_is_lookahead(token_stream, el_RET_KEYWORD))
	{
		err = err || el_match_token(token_stream, el_RET_KEYWORD);
		err = err || el_parse_expr(token_stream);
	}
	else
	{
		int assignment_pos = list->num_statements;
		err = err || el_parse_complex_identifier(token_stream);
		if(el_is_lookahead(token_stream, el_ASSIGN_OPERATOR))
		{
			// TODO - Replace id statement with assignment
			err = err || el_parse_assignment(token_stream, list);
		}
	}
	assert(list->num_statements <= list->max_num_statements);
	return err;
}

static int el_parse_for_statement(struct el_token_stream * token_stream, struct el_ast_statement_list * parent)
{
	DEBUG_PRODUCTION("el_parse_for_statement");
	int err = 0;
	parent->statements[parent->num_statements].type = el_AST_NODE_FOR_STATEMENT;
	struct el_ast_for_statement * for_statement = &parent->statements[parent->num_statements++].for_statement;
	assert(parent->num_statements <= parent->max_num_statements);
	err = err || el_match_token(token_stream, el_FOR_KEYWORD);
	for_statement->index_var_name = el_string_new(el_lookahead(token_stream)->source, -1);
	err = err || el_match_token(token_stream, el_IDENTIFIER); // Index variable
	err = err || el_match_token(token_stream, el_COMMA_SEPARATOR);
	for_statement->value_var_name = el_string_new(el_lookahead(token_stream)->source, -1);
	err = err || el_match_token(token_stream, el_IDENTIFIER); // Element variable
	err = err || el_match_token(token_stream, el_IN_KEYWORD);
	for_statement->list_name = el_string_new("placeholder", -1);
	err = err || el_parse_complex_identifier(token_stream);
	err = err || el_parse_code_block(token_stream, &for_statement->code_block);
	return err;
}

static int el_parse_if_statement(struct el_token_stream * token_stream, struct el_ast_statement_list * parent)
{
	DEBUG_PRODUCTION("el_parse_if_statement");
	int err = 0;
	parent->statements[parent->num_statements].type = el_AST_NODE_IF_STATEMENT;
	struct el_ast_if_statement * if_statement = &parent->statements[parent->num_statements++].if_statement;
	assert(parent->num_statements <= parent->max_num_statements);
	if_statement->num_elif_statements = 0;
	if_statement->max_num_elif_statements = MAX_NUM_ELIFS_PER_IF;
	if_statement->elif_statements = fmalloc(sizeof(struct el_ast_elif_statement) * if_statement->max_num_elif_statements);
	err = err || el_match_token(token_stream, el_IF_KEYWORD);
	if_statement->expression.todo = 0; // TODO
	err = err || el_parse_expr(token_stream);
	err = err || el_parse_code_block(token_stream, &if_statement->code_block);
	if(el_is_lookahead(token_stream, el_ELIF_KEYWORD))
	{
		err = err || el_parse_elif_statements(token_stream, if_statement);
	}
	if(el_is_lookahead(token_stream, el_ELSE_KEYWORD))
	{
		err = err || el_parse_else_statement(token_stream, if_statement);
	}
	return err;
}

static int el_parse_elif_statements(struct el_token_stream * token_stream, struct el_ast_if_statement * parent)
{
	DEBUG_PRODUCTION("el_parse_elif_statements");
	int err = 0;
	struct el_ast_elif_statement * elif_statement = &parent->elif_statements[parent->num_elif_statements++];
	assert(parent->num_elif_statements <= parent->max_num_elif_statements);
	err = err || el_match_token(token_stream, el_ELIF_KEYWORD);
	elif_statement->expression.todo = 0; // TODO
	err = err || el_parse_expr(token_stream);
	err = err || el_parse_code_block(token_stream, &elif_statement->code_block);
	if(el_is_lookahead(token_stream, el_ELIF_KEYWORD))
	{
		err = err || el_parse_elif_statements(token_stream, parent);
	}
	return err;
}

static int el_parse_else_statement(struct el_token_stream * token_stream, struct el_ast_if_statement * parent)
{
	DEBUG_PRODUCTION("el_parse_else_statement");
	int err = 0;
	parent->else_statement = fmalloc(sizeof(struct el_ast_statement_list));
	err = err || el_match_token(token_stream, el_ELSE_KEYWORD);
	err = err || el_parse_code_block(token_stream, parent->else_statement);
	return err;
}

static int el_parse_assignment(struct el_token_stream * token_stream, struct el_ast_statement_list * parent)
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

static int el_parse_data_block(struct el_token_stream * token_stream, struct el_ast_statement_list * parent)
{
	DEBUG_PRODUCTION("el_parse_data_block");
	int err = 0;
	parent->statements[parent->num_statements].type = el_AST_NODE_DATA_BLOCK;
	struct el_ast_data_block * data_block = &parent->statements[parent->num_statements++].data_block;
	assert(parent->num_statements <= parent->max_num_statements);
	data_block->num_var_declarations = 0;
	data_block->max_num_var_declarations = MAX_NUM_VARS_PER_DATA_BLOCK;
	data_block->var_declarations = fmalloc(sizeof(struct el_ast_var_decl) * data_block->max_num_var_declarations);
	err = err || el_match_token(token_stream, el_DAT_KEYWORD);
	data_block->name = el_string_new(el_lookahead(token_stream)->source, -1);
	err = err || el_match_token(token_stream, el_IDENTIFIER);
	err = err || el_match_token(token_stream, el_BLOCK_START);
	err = err || el_parse_data_block_statements(token_stream, data_block);
	err = err || el_match_token(token_stream, el_BLOCK_END);
	err = err || el_parse_new_line(token_stream);
	return err;
}

static int el_parse_data_block_statements(struct el_token_stream * token_stream, struct el_ast_data_block * data_block)
{
	DEBUG_PRODUCTION("el_parse_data_block_statements");
	int err = 0;
	err = err || el_parse_new_lines(token_stream);
	if(el_is_lookahead(token_stream, el_BLOCK_END))
	{
		return err;
	}
	// Parse a single statement
	err = err || el_parse_data_block_statement(token_stream, data_block);
	// Parse additional statements by recursing into this fn
	err = err || el_parse_data_block_statements(token_stream, data_block);
	return err;
}

static int el_parse_data_block_statement(struct el_token_stream * token_stream, struct el_ast_data_block * data_block)
{
	DEBUG_PRODUCTION("el_parse_data_block_statement");
	int err = 0;
	struct el_ast_var_decl * var_decl = &data_block->var_declarations[data_block->num_var_declarations++];
	assert(data_block->num_var_declarations <= data_block->max_num_var_declarations);
	var_decl->name = el_string_new(el_lookahead(token_stream)->source, -1);
	err = err || el_match_token(token_stream, el_IDENTIFIER);
	err = err || el_parse_type(token_stream, &var_decl->type);
	return err;
}

static int el_parse_optional_type(struct el_token_stream * token_stream, struct el_ast_var_type * var_type)
{
	DEBUG_PRODUCTION("el_parse_optional_type");
	int err = 0;
	var_type->is_native = true;
	var_type->native_type = el_NONE;
	if(
		el_is_lookahead(token_stream, el_IDENTIFIER) ||
		el_is_lookahead(token_stream, el_INT_TYPE) ||
		el_is_lookahead(token_stream, el_FLOAT_TYPE))
	{
		err = err || el_parse_type(token_stream, var_type);
	}
	return err;
}

static int el_parse_type(struct el_token_stream * token_stream, struct el_ast_var_type * var_type)
{
	DEBUG_PRODUCTION("el_parse_type");
	int err = 0;
	if(el_is_lookahead(token_stream, el_IDENTIFIER))
	{
		var_type->is_native = false;
		var_type->custom_type = el_string_new(el_lookahead(token_stream)->source, -1);
		err = err || el_match_token(token_stream, el_IDENTIFIER);
	}
	else if(el_is_lookahead(token_stream, el_INT_TYPE))
	{
		var_type->is_native = true;
		var_type->native_type = el_INT_TYPE;
		err = err || el_match_token(token_stream, el_INT_TYPE);
	}
	else if(el_is_lookahead(token_stream, el_FLOAT_TYPE))
	{
		var_type->is_native = true;
		var_type->native_type = el_FLOAT_TYPE;
		err = err || el_match_token(token_stream, el_FLOAT_TYPE);
	}
	else
	{
		fprintf(stderr, "Expected a type, got %d %s\n", token_stream->tokens[token_stream->current_token].type, token_stream->tokens[token_stream->current_token].source);
		err = 1;
	}
	
	// Parse multiple slice open & close tokens to support multi-dimensional slices
	var_type->num_dimensions = 0;
	while(el_is_lookahead(token_stream, el_SLICE_START) && err == 0)
	{
		var_type->num_dimensions++;
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

static struct el_token * el_lookahead(struct el_token_stream * token_stream)
{
	if(token_stream->current_token >= token_stream->num_tokens)
	{
		printf("Failed to get lookahead, ran out of tokens");
		return NULL;
	}

	return &token_stream->tokens[token_stream->current_token];
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
