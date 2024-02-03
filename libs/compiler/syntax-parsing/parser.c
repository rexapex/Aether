#include "parser.h"
#include <allocators/fmalloc.h>
#include <allocators/linear-allocator.h>
#include <compiler/error.h>
#include <compiler/lexing/token-stream.h>
#include <containers/string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#if 0
	#define DEBUG_PRODUCTION(x) do { printf("%s\n", x); } while(0);
#else
	#define DEBUG_PRODUCTION(x) do {} while(0);
#endif

#define DEBUG_TOKEN_MATCHING 0

#define ALLOCATOR_CAPACITY (10 * 1024 * 1024) // 10MiB

#define MAX_NUM_NODES_PER_NODE_LIST 256
#define MAX_NUM_EXPRS_PER_EXPR_LIST 128
#define MAX_NUM_PARAMS_PER_PARAM_LIST 8
#define MAX_NUM_ELIFS_PER_IF 8
#define MAX_NUM_VARS_PER_DATA_BLOCK 16

static int el_parse_new_line(struct el_token_stream * token_stream);
static int el_parse_new_lines(struct el_token_stream * token_stream);
static int el_parse_statements(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_statement_list * list);
static int el_parse_statement(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_statement_list * list);

static int el_parse_function(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_statement_list * parent);
static int el_parse_parameter_list(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_parameter_list * parameter_list);
static int el_parse_parameters(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_parameter_list * parameter_list);
static int el_parse_parameter(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_var_decl * var_decl);

static int el_parse_code_block(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_statement_list * list);
static int el_parse_code_block_statements(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_statement_list * list);
static int el_parse_code_block_statement(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_statement_list * list);

static int el_parse_for_statement(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_statement_list * parent);
static int el_parse_if_statement(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_statement_list * parent);
static int el_parse_elif_statements(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_if_statement * parent);
static int el_parse_else_statement(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_if_statement * parent);

static int el_parse_assignment(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_expression * expression);
static int el_parse_expr(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_expression * expression);
static int el_parse_function_call(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_expression_list * expression_list);
static int el_parse_arguments(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_expression_list * expression_list);

static int el_parse_data_block(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_statement_list * parent);
static int el_parse_data_block_statements(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_data_block * data_block);
static int el_parse_data_block_statement(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_data_block * data_block);

static int el_parse_optional_type(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_var_type * var_type);
static int el_parse_type(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_var_type * var_type);

static int el_parse_complex_identifier(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_expression * expression);
static int el_parse_complex_identifier_inner(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_expression * expression);

static int el_parse_boolean_or_expr(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_expression * expression);
static int el_parse_boolean_and_expr(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_expression * expression);
static int el_parse_add_or_sub_expr(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_expression * expression);
static int el_parse_mult_or_div_expr(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_expression * expression);
static int el_parse_factor_expr(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_expression * expression);

static int el_convert_to_binary_op(struct el_linear_allocator * allocator, struct el_ast_expression * expression, int type);
static int el_new_expr_list(struct el_linear_allocator * allocator, struct el_ast_expression * expression, int type);

static int el_match_token(struct el_token_stream * token_stream, int type);
static struct el_token * el_lookahead(struct el_token_stream * token_stream);
static bool el_is_lookahead(struct el_token_stream * token_stream, int type);
static el_string el_copy_lookahead(struct el_token_stream * token_stream, struct el_linear_allocator * allocator);

struct el_ast el_parse_token_stream(struct el_token_stream * token_stream)
{
	struct el_ast ast = {
		.allocator.memory = fmalloc(ALLOCATOR_CAPACITY),
		.allocator.capacity = ALLOCATOR_CAPACITY,
		.allocator.size = 0,
		.root.statements = NULL,
		.root.max_num_statements = MAX_NUM_NODES_PER_NODE_LIST,
		.root.num_statements = 0
	};
	ast.root.statements = el_linear_alloc(&ast.allocator, sizeof(struct el_ast_statement) * MAX_NUM_NODES_PER_NODE_LIST);

	if(!ast.root.statements)
	{
		fprintf(stderr, "Failed to allocate root ast statements node\n");
		return ast;
	}

	if(el_parse_statements(token_stream, &ast.allocator, &ast.root) != 0)
	{
		fprintf(stderr, "Failed to parse token stream\n");
		el_ast_delete(&ast);
	}
	else
	{
		el_ast_print(&ast);
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

static int el_parse_statements(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_statement_list * list)
{
	DEBUG_PRODUCTION("el_parse_statements");
	int err = 0;
	if(token_stream->current_token < token_stream->num_tokens)
	{
		// Parse a single statement
		err = err || el_parse_new_lines(token_stream);
		err = err || el_parse_statement(token_stream, allocator, list);
		err = err || el_parse_new_lines(token_stream);

		// Parse additional statements by recursing into this fn
		err = err || el_parse_statements(token_stream, allocator, list);
	}
	return err;
}

static int el_parse_statement(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_statement_list * list)
{
	DEBUG_PRODUCTION("el_parse_statement");
	int err = 0;
	if(el_is_lookahead(token_stream, el_FNC_KEYWORD))
	{
		err = err || el_parse_function(token_stream, allocator, list);
	}
	else if(el_is_lookahead(token_stream, el_DAT_KEYWORD))
	{
		err = err || el_parse_data_block(token_stream, allocator, list);
	}
	else
	{
		// Statements which are valid within code blocks are also valid at file scope
		err = err || el_parse_code_block_statement(token_stream, allocator, list);
	}
	return err;
}

static int el_parse_function(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_statement_list * parent)
{
	DEBUG_PRODUCTION("el_parse_function");
	int err = 0;
	assert(parent->num_statements < parent->max_num_statements);
	parent->statements[parent->num_statements].type = el_AST_NODE_FUNCTION_DEFINITION;
	struct el_ast_function_definition * function_definition = &parent->statements[parent->num_statements++].function_definition;

	err = err || el_match_token(token_stream, el_FNC_KEYWORD);
	function_definition->name = el_copy_lookahead(token_stream, allocator);
	if(!function_definition->name)
		return el_ALLOCATION_ERROR;

	err = err || el_match_token(token_stream, el_IDENTIFIER);
	err = err || el_parse_parameter_list(token_stream, allocator, &function_definition->parameter_list);
	err = err || el_parse_optional_type(token_stream, allocator, &function_definition->return_type);
	err = err || el_parse_code_block(token_stream, allocator, &function_definition->code_block);
	return err;
}

static int el_parse_parameter_list(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_parameter_list * parameter_list)
{
	DEBUG_PRODUCTION("el_parse_parameter_list");
	int err = 0;
	parameter_list->parameters = el_linear_alloc(allocator, sizeof(struct el_ast_var_decl) * MAX_NUM_PARAMS_PER_PARAM_LIST);
	if(!parameter_list->parameters)
		return el_ALLOCATION_ERROR;
	parameter_list->max_num_parameters = MAX_NUM_PARAMS_PER_PARAM_LIST;
	parameter_list->num_parameters = 0;
	err = err || el_match_token(token_stream, el_PARENTHESIS_OPEN);
	err = err || el_parse_parameters(token_stream, allocator, parameter_list);
	err = err || el_match_token(token_stream, el_PARENTHESIS_CLOSE);
	return err;
}

static int el_parse_parameters(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_parameter_list * parameter_list)
{
	DEBUG_PRODUCTION("el_parse_parameters");
	int err = 0;
	if(el_is_lookahead(token_stream, el_PARENTHESIS_CLOSE))
		return err;

	assert(parameter_list->num_parameters < parameter_list->max_num_parameters);
	err = err || el_parse_parameter(token_stream, allocator, &parameter_list->parameters[parameter_list->num_parameters++]);
	if(el_is_lookahead(token_stream, el_COMMA_SEPARATOR))
	{
		// NOTE - This allows a trailing comma before the closing bracket
		err = err || el_match_token(token_stream, el_COMMA_SEPARATOR);
		err = err || el_parse_parameters(token_stream, allocator, parameter_list);
	}
	return err;
}

static int el_parse_parameter(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_var_decl * var_decl)
{
	DEBUG_PRODUCTION("el_parse_parameter");
	int err = 0;
	var_decl->name = el_copy_lookahead(token_stream, allocator);
	if(!var_decl->name)
		return el_ALLOCATION_ERROR;
	err = err || el_match_token(token_stream, el_IDENTIFIER);
	err = err || el_parse_optional_type(token_stream, allocator, &var_decl->type);
	return err;
}

static int el_parse_code_block(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_statement_list * list)
{
	DEBUG_PRODUCTION("el_parse_code_block");
	int err = 0;
	list->num_statements = 0;
	list->max_num_statements = MAX_NUM_NODES_PER_NODE_LIST;
	list->statements = el_linear_alloc(allocator, sizeof(struct el_ast_statement) * list->max_num_statements);
	if(!list->statements)
		return el_ALLOCATION_ERROR;

	err = err || el_match_token(token_stream, el_BLOCK_START);
	err = err || el_parse_code_block_statements(token_stream, allocator, list);
	err = err || el_match_token(token_stream, el_BLOCK_END);
	err = err || el_parse_new_line(token_stream);
	return err;
}

static int el_parse_code_block_statements(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_statement_list * list)
{
	DEBUG_PRODUCTION("el_parse_code_block_statements");
	int err = 0;
	err = err || el_parse_new_lines(token_stream);
	if(el_is_lookahead(token_stream, el_BLOCK_END))
	{
		return err;
	}
	// Parse a single statement
	err = err || el_parse_code_block_statement(token_stream, allocator, list);
	// Parse additional statements by recursing into this fn
	err = err || el_parse_code_block_statements(token_stream, allocator, list);
	return err;
}

static int el_parse_code_block_statement(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_statement_list * list)
{
	DEBUG_PRODUCTION("el_parse_code_block_statement");
	int err = 0;
	assert(list->num_statements < list->max_num_statements);
	if(el_is_lookahead(token_stream, el_FOR_KEYWORD))
	{
		err = err || el_parse_for_statement(token_stream, allocator, list);
	}
	else if(el_is_lookahead(token_stream, el_IF_KEYWORD))
	{
		err = err || el_parse_if_statement(token_stream, allocator, list);
	}
	else if(el_is_lookahead(token_stream, el_RET_KEYWORD))
	{
		int statement_index = list->num_statements++;
		err = err || el_match_token(token_stream, el_RET_KEYWORD);
		list->statements[statement_index].type = el_AST_NODE_RETURN_STATEMENT;
		err = err || el_parse_expr(token_stream, allocator, &list->statements[statement_index].return_statement.expression);
	}
	else
	{
		int statement_index = list->num_statements++;
		list->statements[statement_index].type = el_AST_NODE_EXPRESSION;
		err = err || el_parse_complex_identifier(token_stream, allocator, &list->statements[statement_index].expression);
		if(el_is_lookahead(token_stream, el_ASSIGN_OPERATOR))
		{
			// Move the identifier node into the lhs of an assignment node
			list->statements[statement_index].assignment.lhs = list->statements[statement_index].expression;
			list->statements[statement_index].type = el_AST_NODE_ASSIGNMENT;
			err = err || el_parse_assignment(token_stream, allocator, &list->statements[statement_index].assignment.rhs);
		}
	}
	return err;
}

static int el_parse_for_statement(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_statement_list * parent)
{
	DEBUG_PRODUCTION("el_parse_for_statement");
	int err = 0;
	assert(parent->num_statements < parent->max_num_statements);
	parent->statements[parent->num_statements].type = el_AST_NODE_FOR_STATEMENT;
	struct el_ast_for_statement * for_statement = &parent->statements[parent->num_statements++].for_statement;

	err = err || el_match_token(token_stream, el_FOR_KEYWORD);
	for_statement->index_var_name = el_copy_lookahead(token_stream, allocator);
	if(!for_statement->index_var_name)
		return el_ALLOCATION_ERROR;

	err = err || el_match_token(token_stream, el_IDENTIFIER); // Index variable
	err = err || el_match_token(token_stream, el_COMMA_SEPARATOR);
	for_statement->value_var_name = el_copy_lookahead(token_stream, allocator);
	if(!for_statement->value_var_name)
		return el_ALLOCATION_ERROR;

	err = err || el_match_token(token_stream, el_IDENTIFIER); // Element variable
	err = err || el_match_token(token_stream, el_IN_KEYWORD);
	err = err || el_parse_complex_identifier(token_stream, allocator, &for_statement->range);
	err = err || el_parse_code_block(token_stream, allocator, &for_statement->code_block);
	return err;
}

static int el_parse_if_statement(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_statement_list * parent)
{
	DEBUG_PRODUCTION("el_parse_if_statement");
	int err = 0;
	assert(parent->num_statements < parent->max_num_statements);
	parent->statements[parent->num_statements].type = el_AST_NODE_IF_STATEMENT;
	struct el_ast_if_statement * if_statement = &parent->statements[parent->num_statements++].if_statement;
	
	if_statement->num_elif_statements = 0;
	if_statement->max_num_elif_statements = MAX_NUM_ELIFS_PER_IF;
	if_statement->elif_statements = el_linear_alloc(allocator, sizeof(struct el_ast_elif_statement) * if_statement->max_num_elif_statements);
	if(!if_statement->elif_statements)
		return el_ALLOCATION_ERROR;

	err = err || el_match_token(token_stream, el_IF_KEYWORD);
	err = err || el_parse_expr(token_stream, allocator, &if_statement->expression);
	err = err || el_parse_code_block(token_stream, allocator, &if_statement->code_block);
	if(el_is_lookahead(token_stream, el_ELIF_KEYWORD))
	{
		err = err || el_parse_elif_statements(token_stream, allocator, if_statement);
	}
	if(el_is_lookahead(token_stream, el_ELSE_KEYWORD))
	{
		err = err || el_parse_else_statement(token_stream, allocator, if_statement);
	}
	return err;
}

static int el_parse_elif_statements(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_if_statement * parent)
{
	DEBUG_PRODUCTION("el_parse_elif_statements");
	int err = 0;
	assert(parent->num_elif_statements < parent->max_num_elif_statements);
	struct el_ast_elif_statement * elif_statement = &parent->elif_statements[parent->num_elif_statements++];
	err = err || el_match_token(token_stream, el_ELIF_KEYWORD);
	err = err || el_parse_expr(token_stream, allocator, &elif_statement->expression);
	err = err || el_parse_code_block(token_stream, allocator, &elif_statement->code_block);
	if(el_is_lookahead(token_stream, el_ELIF_KEYWORD))
	{
		err = err || el_parse_elif_statements(token_stream, allocator, parent);
	}
	return err;
}

static int el_parse_else_statement(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_if_statement * parent)
{
	DEBUG_PRODUCTION("el_parse_else_statement");
	int err = 0;
	parent->else_statement = el_linear_alloc(allocator, sizeof(struct el_ast_statement_list));
	if(!parent->else_statement)
		return el_ALLOCATION_ERROR;
	err = err || el_match_token(token_stream, el_ELSE_KEYWORD);
	err = err || el_parse_code_block(token_stream, allocator, parent->else_statement);
	return err;
}

static int el_parse_assignment(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_expression * expression)
{
	DEBUG_PRODUCTION("el_parse_assignment");
	int err = 0;
	err = err || el_match_token(token_stream, el_ASSIGN_OPERATOR);
	err = err || el_parse_expr(token_stream, allocator, expression);
	return err;
}

static int el_parse_expr(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_expression * expression)
{
	DEBUG_PRODUCTION("el_parse_expr");
	int err = 0;
	err = err || el_parse_boolean_or_expr(token_stream, allocator, expression);
	if(
		el_is_lookahead(token_stream, el_EQUALS_COMPARATOR) ||
		el_is_lookahead(token_stream, el_GREATER_THAN_COMPARATOR) ||
		el_is_lookahead(token_stream, el_LESS_THAN_COMPARATOR) ||
		el_is_lookahead(token_stream, el_LEQUALS_COMPARATOR) ||
		el_is_lookahead(token_stream, el_GEQUALS_COMPARATOR))
	{
		if(el_is_lookahead(token_stream, el_EQUALS_COMPARATOR))
		{
			err = err || el_convert_to_binary_op(allocator, expression, el_AST_EXPR_EQUALS);
			err = err || el_match_token(token_stream, el_EQUALS_COMPARATOR);
			err = err || el_parse_expr(token_stream, allocator, expression->binary_op.rhs);
		}
		else if(el_is_lookahead(token_stream, el_GREATER_THAN_COMPARATOR))
		{
			err = err || el_convert_to_binary_op(allocator, expression, el_AST_EXPR_GREATER_THAN);
			err = err || el_match_token(token_stream, el_GREATER_THAN_COMPARATOR);
			err = err || el_parse_expr(token_stream, allocator, expression->binary_op.rhs);
		}
		else if(el_is_lookahead(token_stream, el_LESS_THAN_COMPARATOR))
		{
			err = err || el_convert_to_binary_op(allocator, expression, el_AST_EXPR_LESS_THAN);
			err = err || el_match_token(token_stream, el_LESS_THAN_COMPARATOR);
			err = err || el_parse_expr(token_stream, allocator, expression->binary_op.rhs);
		}
		else if(el_is_lookahead(token_stream, el_LEQUALS_COMPARATOR))
		{
			err = err || el_convert_to_binary_op(allocator, expression, el_AST_EXPR_LEQUALS);
			err = err || el_match_token(token_stream, el_LEQUALS_COMPARATOR);
			err = err || el_parse_expr(token_stream, allocator, expression->binary_op.rhs);
		}
		else if(el_is_lookahead(token_stream, el_GEQUALS_COMPARATOR))
		{
			err = err || el_convert_to_binary_op(allocator, expression, el_AST_EXPR_GEQUALS);
			err = err || el_match_token(token_stream, el_GEQUALS_COMPARATOR);
			err = err || el_parse_expr(token_stream, allocator, expression->binary_op.rhs);
		}
	}
	return err;
}

static int el_parse_function_call(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_expression_list * expression_list)
{
	DEBUG_PRODUCTION("el_parse_function_call");
	int err = 0;
	err = err || el_match_token(token_stream, el_PARENTHESIS_OPEN);
	err = err || el_parse_arguments(token_stream, allocator, expression_list);
	err = err || el_match_token(token_stream, el_PARENTHESIS_CLOSE);
	return err;
}

static int el_parse_arguments(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_expression_list * expression_list)
{
	DEBUG_PRODUCTION("el_parse_arguments");
	int err = 0;
	if(el_is_lookahead(token_stream, el_PARENTHESIS_CLOSE))
		return err;

	assert(expression_list->num_expressions < expression_list->max_num_expressions);
	err = err || el_parse_expr(token_stream, allocator, &expression_list->expressions[expression_list->num_expressions++]);
	if(el_is_lookahead(token_stream, el_COMMA_SEPARATOR))
	{
		// NOTE - This allows a trailing comma before the closing bracket
		err = err || el_match_token(token_stream, el_COMMA_SEPARATOR);
		err = err || el_parse_arguments(token_stream, allocator, expression_list);
	}
	return err;
}

static int el_parse_data_block(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_statement_list * parent)
{
	DEBUG_PRODUCTION("el_parse_data_block");
	int err = 0;
	assert(parent->num_statements < parent->max_num_statements);
	parent->statements[parent->num_statements].type = el_AST_NODE_DATA_BLOCK;
	struct el_ast_data_block * data_block = &parent->statements[parent->num_statements++].data_block;

	data_block->num_var_declarations = 0;
	data_block->max_num_var_declarations = MAX_NUM_VARS_PER_DATA_BLOCK;
	data_block->var_declarations = el_linear_alloc(allocator, sizeof(struct el_ast_var_decl) * data_block->max_num_var_declarations);
	if(!data_block->var_declarations)
		return el_ALLOCATION_ERROR;

	err = err || el_match_token(token_stream, el_DAT_KEYWORD);
	data_block->name = el_copy_lookahead(token_stream, allocator);
	if(!data_block->name)
		return el_ALLOCATION_ERROR;

	err = err || el_match_token(token_stream, el_IDENTIFIER);
	err = err || el_match_token(token_stream, el_BLOCK_START);
	err = err || el_parse_data_block_statements(token_stream, allocator, data_block);
	err = err || el_match_token(token_stream, el_BLOCK_END);
	err = err || el_parse_new_line(token_stream);
	return err;
}

static int el_parse_data_block_statements(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_data_block * data_block)
{
	DEBUG_PRODUCTION("el_parse_data_block_statements");
	int err = 0;
	err = err || el_parse_new_lines(token_stream);
	if(el_is_lookahead(token_stream, el_BLOCK_END))
		return err;

	// Parse a single statement
	err = err || el_parse_data_block_statement(token_stream, allocator, data_block);
	// Parse additional statements by recursing into this fn
	err = err || el_parse_data_block_statements(token_stream, allocator, data_block);
	return err;
}

static int el_parse_data_block_statement(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_data_block * data_block)
{
	DEBUG_PRODUCTION("el_parse_data_block_statement");
	int err = 0;
	assert(data_block->num_var_declarations < data_block->max_num_var_declarations);
	struct el_ast_var_decl * var_decl = &data_block->var_declarations[data_block->num_var_declarations++];
	var_decl->name = el_copy_lookahead(token_stream, allocator);
	if(!var_decl->name)
		return el_ALLOCATION_ERROR;
	err = err || el_match_token(token_stream, el_IDENTIFIER);
	err = err || el_parse_type(token_stream, allocator, &var_decl->type);
	return err;
}

static int el_parse_optional_type(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_var_type * var_type)
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
		err = err || el_parse_type(token_stream, allocator, var_type);
	}
	return err;
}

static int el_parse_type(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_var_type * var_type)
{
	DEBUG_PRODUCTION("el_parse_type");
	int err = 0;
	if(el_is_lookahead(token_stream, el_IDENTIFIER))
	{
		var_type->is_native = false;
		var_type->custom_type = el_copy_lookahead(token_stream, allocator);
		if(!var_type->custom_type)
			return el_ALLOCATION_ERROR;
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
		return el_EXPECTED_TYPE_PARSE_ERROR;
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

static int el_parse_complex_identifier(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_expression * expression)
{
	DEBUG_PRODUCTION("el_parse_complex_identifier");
	int err = 0;
	expression->type = el_AST_EXPR_IDENTIFIER;
	expression->identifier = el_copy_lookahead(token_stream, allocator);
	if(!expression->identifier)
		return el_ALLOCATION_ERROR;
	err = err || el_match_token(token_stream, el_IDENTIFIER);
	err = err || el_parse_complex_identifier_inner(token_stream, allocator, expression);
	return err;
}

static int el_parse_complex_identifier_inner(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_expression * expression)
{
	DEBUG_PRODUCTION("el_parse_complex_identifier_inner");
	int err = 0;
	if(el_is_lookahead(token_stream, el_SLICE_START))
	{
		err = err || el_convert_to_binary_op(allocator, expression, el_AST_EXPR_SLICE_INDEX);
		err = err || el_match_token(token_stream, el_SLICE_START);
		err = err || el_parse_expr(token_stream, allocator, expression->binary_op.rhs);
		err = err || el_match_token(token_stream, el_SLICE_END);
	}
	else if(el_is_lookahead(token_stream, el_PARENTHESIS_OPEN))
	{
		err = err || el_convert_to_binary_op(allocator, expression, el_AST_EXPR_FUNCTION_CALL);
		err = err || el_new_expr_list(allocator, expression->binary_op.rhs, el_AST_EXPR_ARGUMENTS);
		err = err || el_parse_function_call(token_stream, allocator, expression->binary_op.rhs->expression_list);
	}
	else if(el_is_lookahead(token_stream, el_DOT_OPERATOR))
	{
		err = err || el_convert_to_binary_op(allocator, expression, el_AST_EXPR_DOT);
		err = err || el_match_token(token_stream, el_DOT_OPERATOR);
		expression->binary_op.rhs->type = el_AST_EXPR_IDENTIFIER;
		expression->binary_op.rhs->identifier = el_copy_lookahead(token_stream, allocator);
		if(!expression->binary_op.rhs->identifier)
			return el_ALLOCATION_ERROR;
		err = err || el_match_token(token_stream, el_IDENTIFIER);
	}
	else
	{
		// NOTE - This is not an error condition
		return err;
	}
	// Recurse into fn s.t. slice indexes, function calls, and dot operations can be chained
	err = err || el_parse_complex_identifier_inner(token_stream, allocator, expression->binary_op.rhs);
	return err;
}

static int el_parse_boolean_or_expr(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_expression * expression)
{
	DEBUG_PRODUCTION("el_parse_boolean_or_expr");
	int err = 0;
	err = err || el_parse_boolean_and_expr(token_stream, allocator, expression);
	if(el_is_lookahead(token_stream, el_BOOLEAN_OR))
	{
		err = err || el_convert_to_binary_op(allocator, expression, el_AST_EXPR_BOOLEAN_OR);
		err = err || el_match_token(token_stream, el_BOOLEAN_OR);
		err = err || el_parse_boolean_or_expr(token_stream, allocator, expression->binary_op.rhs);
	}
	return err;
}

static int el_parse_boolean_and_expr(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_expression * expression)
{
	DEBUG_PRODUCTION("el_parse_boolean_and_expr");
	int err = 0;
	err = err || el_parse_add_or_sub_expr(token_stream, allocator, expression);
	if(el_is_lookahead(token_stream, el_BOOLEAN_AND))
	{
		err = err || el_convert_to_binary_op(allocator, expression, el_AST_EXPR_BOOLEAN_AND);
		err = err || el_match_token(token_stream, el_BOOLEAN_AND);
		err = err || el_parse_boolean_and_expr(token_stream, allocator, expression->binary_op.rhs);
	}
	return err;
}

static int el_parse_add_or_sub_expr(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_expression * expression)
{
	DEBUG_PRODUCTION("el_parse_add_or_sub_expr");
	int err = 0;
	err = err || el_parse_mult_or_div_expr(token_stream, allocator, expression);
	if(el_is_lookahead(token_stream, el_ADD_OPERATOR))
	{
		err = err || el_convert_to_binary_op(allocator, expression, el_AST_EXPR_ADD);
		err = err || el_match_token(token_stream, el_ADD_OPERATOR);
		err = err || el_parse_add_or_sub_expr(token_stream, allocator, expression->binary_op.rhs);
	}
	else if(el_is_lookahead(token_stream, el_SUBTRACT_OPERATOR))
	{
		err = err || el_convert_to_binary_op(allocator, expression, el_AST_EXPR_SUB);
		err = err || el_match_token(token_stream, el_SUBTRACT_OPERATOR);
		err = err || el_parse_add_or_sub_expr(token_stream, allocator, expression->binary_op.rhs);
	}
	return err;
}

static int el_parse_mult_or_div_expr(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_expression * expression)
{
	DEBUG_PRODUCTION("el_parse_mult_or_div_expr");
	int err = 0;
	err = err || el_parse_factor_expr(token_stream, allocator, expression);
	if(el_is_lookahead(token_stream, el_MULTIPLY_OPERATOR))
	{
		err = err || el_convert_to_binary_op(allocator, expression, el_AST_EXPR_MUL);
		err = err || el_match_token(token_stream, el_MULTIPLY_OPERATOR);
		err = err || el_parse_mult_or_div_expr(token_stream, allocator, expression->binary_op.rhs);
	}
	else if(el_is_lookahead(token_stream, el_DIVIDE_OPERATOR))
	{
		err = err || el_convert_to_binary_op(allocator, expression, el_AST_EXPR_DIV);
		err = err || el_match_token(token_stream, el_DIVIDE_OPERATOR);
		err = err || el_parse_mult_or_div_expr(token_stream, allocator, expression->binary_op.rhs);
	}
	return err;
}

static int el_parse_factor_expr(struct el_token_stream * token_stream, struct el_linear_allocator * allocator, struct el_ast_expression * expression)
{
	DEBUG_PRODUCTION("el_parse_factor_expr");
	int err = 0;
	if(el_is_lookahead(token_stream, el_NUMBER_LITERAL))
	{
		expression->type = el_AST_EXPR_NUMBER_LITERAL;
		expression->number_literal = el_copy_lookahead(token_stream, allocator);
		if(!expression->number_literal)
			return el_ALLOCATION_ERROR;
		err = err || el_match_token(token_stream, el_NUMBER_LITERAL);
	}
	else if(el_is_lookahead(token_stream, el_STRING_LITERAL))
	{
		expression->type = el_AST_EXPR_STRING_LITERAL;
		expression->string_literal = el_copy_lookahead(token_stream, allocator);
		if(!expression->string_literal)
			return el_ALLOCATION_ERROR;
		err = err || el_match_token(token_stream, el_STRING_LITERAL);
	}
	else if(el_is_lookahead(token_stream, el_IDENTIFIER))
	{
		err = err || el_parse_complex_identifier(token_stream, allocator, expression);
	}
	else if(el_is_lookahead(token_stream, el_PARENTHESIS_OPEN))
	{
		err = err || el_match_token(token_stream, el_PARENTHESIS_OPEN);
		err = err || el_parse_expr(token_stream, allocator, expression);
		err = err || el_match_token(token_stream, el_PARENTHESIS_CLOSE);
	}
	else if(el_is_lookahead(token_stream, el_SLICE_START))
	{
		err = err || el_new_expr_list(allocator, expression, el_AST_EXPR_SLICE_LITERAL);
		err = err || el_match_token(token_stream, el_SLICE_START);
		err = err || el_parse_arguments(token_stream, allocator, expression->expression_list);
		err = err || el_match_token(token_stream, el_SLICE_END);
	}
	else
	{
		fprintf(stderr, "Expected a factor expression\n");
		return el_EXPECTED_FACTOR_EXPR_PARSE_ERROR;
	}
	return err;
}

int el_convert_to_binary_op(struct el_linear_allocator * allocator, struct el_ast_expression * expression, int type)
{
	struct el_ast_expression lhs = *expression;
	expression->type = type;
	expression->binary_op.lhs = el_linear_alloc(allocator, sizeof *expression);
	expression->binary_op.rhs = el_linear_alloc(allocator, sizeof *expression);
	if(!expression->binary_op.lhs || !expression->binary_op.rhs)
	{
		// NOTE - Don't need to call ffree as ast will be freed on error
		return el_ALLOCATION_ERROR;
	}
	*expression->binary_op.lhs = lhs;
	return 0;
}

int el_new_expr_list(struct el_linear_allocator * allocator, struct el_ast_expression * expression, int type)
{
	expression->type = type;
	expression->expression_list = el_linear_alloc(allocator, sizeof(struct el_ast_expression_list));
	if(!expression->expression_list)
	{
		return el_ALLOCATION_ERROR;
	}
	expression->expression_list->expressions = el_linear_alloc(allocator, sizeof(struct el_ast_expression) * MAX_NUM_EXPRS_PER_EXPR_LIST);
	if(!expression->expression_list->expressions)
	{
		// NOTE - Don't need to call ffree as ast will be freed on error
		return el_ALLOCATION_ERROR;
	}
	expression->expression_list->max_num_expressions = MAX_NUM_EXPRS_PER_EXPR_LIST;
	expression->expression_list->num_expressions = 0;
	return 0;
}

// Match the current lookahead token to the type given
// If the types do not match, a non-zero error code is returned
static int el_match_token(struct el_token_stream * token_stream, int type)
{
	int lookahead = token_stream->tokens[token_stream->current_token].type;
	if(lookahead == type)
	{
	#if DEBUG_TOKEN_MATCHING
		printf("Matched token: %d %s\n", type, token_stream->tokens[token_stream->current_token].source);
	#endif
		++token_stream->current_token;
		return 0;
	}

	fprintf(stderr, "Failed to match token at lookahead index %d: expected %d, got %d\n", token_stream->current_token, type, lookahead);
	return el_MATCH_TOKEN_PARSE_ERROR;
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

static el_string el_copy_lookahead(struct el_token_stream * token_stream, struct el_linear_allocator * allocator)
{
	el_string source = el_lookahead(token_stream)->source;
	int num_bytes = el_string_byte_size(source);
	void * memory = el_linear_alloc(allocator, num_bytes);
	return el_string_inplace_new(memory, num_bytes, source, -1);
}
