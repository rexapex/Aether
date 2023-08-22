#pragma once
#include <compiler/lexing/token-stream.h>
#include <containers/string.h>

enum el_ast_statement_type
{
	el_AST_NODE_DATA_BLOCK,
	el_AST_NODE_FUNCTION_DEFINITION,
	el_AST_NODE_FOR_STATEMENT,
	el_AST_NODE_IF_STATEMENT,
	el_AST_NODE_ASSIGNMENT,
};

struct el_ast_statement_list
{
	struct el_ast_statement * statements;
	int max_num_statements;
	int num_statements;
};

struct el_ast_var_type
{
	bool is_native;
	union
	{
		int native_type;
		el_string custom_type;
	};
	int num_dimensions; // 0 for single, 1 for slice, 2 for 2-dimensional slice, etc.
};

struct el_ast_var_decl
{
	el_string name;
	struct el_ast_var_type type;
};

struct el_ast_expression
{
	int todo;
};

struct el_ast_data_block
{
	el_string name;
	struct el_ast_var_decl * var_declarations;
	int max_num_var_declarations;
	int num_var_declarations;
};

struct el_ast_function_definition
{
	el_string name;
	// TODO - Parameters
	struct el_ast_var_type return_type;
	struct el_ast_statement_list code_block;
};

struct el_ast_for_statement
{
	el_string index_var_name;
	el_string value_var_name;
	el_string list_name;
	struct el_ast_statement_list code_block;
};

struct el_ast_elif_statement
{
	struct el_ast_expression expression;
	struct el_ast_statement_list code_block;
};

struct el_ast_if_statement
{
	struct el_ast_expression expression;
	struct el_ast_statement_list code_block;
	struct el_ast_elif_statement * elif_statements;
	struct el_ast_statement_list * else_statement;
	int max_num_elif_statements;
	int num_elif_statements;
};

// TODO
struct el_ast_assignment
{
	int lhs;
	int rhs;
};

struct el_ast_statement
{
	int type;

	union
	{
		struct el_ast_data_block data_block;
		struct el_ast_function_definition function_definition;
		struct el_ast_for_statement for_statement;
		struct el_ast_if_statement if_statement;
		struct el_ast_assignment assignment;
	};
};

struct el_ast
{
	struct el_ast_statement_list root;
};

void el_ast_print(struct el_ast * ast);

// Delete the ast and its decendant nodes
void el_ast_delete(struct el_ast * ast);
