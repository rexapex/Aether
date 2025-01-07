#pragma once
#include <allocators/linear-allocator.h>
#include <compiler/lexing/token-stream.h>
#include <containers/string.h>

enum el_ast_statement_type
{
	el_AST_NODE_DATA_BLOCK,
	el_AST_NODE_FUNCTION_DEFINITION,
	el_AST_NODE_FOR_STATEMENT,
	el_AST_NODE_IF_STATEMENT,
	el_AST_NODE_ASSIGNMENT,
	el_AST_NODE_RETURN_STATEMENT,
	el_AST_NODE_EXPRESSION
};

enum el_ast_expression_type
{
	el_AST_EXPR_EQUALS,
	el_AST_EXPR_GREATER_THAN,
	el_AST_EXPR_LESS_THAN,
	el_AST_EXPR_GEQUALS,
	el_AST_EXPR_LEQUALS,

	el_AST_EXPR_BOOLEAN_AND,
	el_AST_EXPR_BOOLEAN_OR,

	el_AST_EXPR_ADD,
	el_AST_EXPR_SUB,
	el_AST_EXPR_MUL,
	el_AST_EXPR_DIV,

	el_AST_EXPR_DOT,

	el_AST_EXPR_FUNCTION_CALL,
	el_AST_EXPR_LIST_INDEX,

	el_AST_EXPR_NUMBER_LITERAL,
	el_AST_EXPR_STRING_LITERAL,
	el_AST_EXPR_LIST_LITERAL,

	el_AST_EXPR_ARGUMENTS,

	el_AST_EXPR_IDENTIFIER,

	el_AST_EXPR_MATCH
};

enum el_ast_pattern_type
{
	el_PATTERN_WILDCARD,

	el_PATTERN_NUMBER_LITERAL,
	el_PATTERN_STRING_LITERAL,
	el_PATTERN_LIST,

	el_PATTERN_IDENTIFIER
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
	int num_dimensions; // 0 for single, 1 for list, 2 for 2-dimensional list, etc.
};

struct el_ast_var_decl
{
	el_string name;
	struct el_ast_var_type type;
};

struct el_ast_pattern
{
	int type;
	union
	{
		el_string number_literal;
		el_string string_literal;
		el_string identifier;
		// TODO - List pattern
		// TODO - Tuple pattern
	};
};

struct el_ast_expression;

struct el_ast_expression_list
{
	struct el_ast_expression * expressions;
	int max_num_expressions;
	int num_expressions;
};

struct el_ast_match_clause;

struct el_ast_expression
{
	int type;
	union
	{
		el_string number_literal;
		el_string string_literal;
		el_string identifier;

		struct el_ast_expression_list * expression_list;

		struct
		{
			struct el_ast_expression * lhs;
			struct el_ast_expression * rhs;
		} binary_op;

		struct
		{
			struct el_ast_expression * argument;
			struct el_ast_match_clause * clauses;
			int max_num_clauses;
			int num_clauses;
		} match;
	};
};

struct el_ast_match_clause
{
	struct el_ast_pattern pattern;
	struct el_ast_expression expression;
};

struct el_ast_data_block
{
	el_string name;
	struct el_ast_var_decl * var_declarations;
	int max_num_var_declarations;
	int num_var_declarations;
};

struct el_ast_parameter_list
{
	struct el_ast_var_decl * parameters;
	int max_num_parameters;
	int num_parameters;
};

struct el_ast_function_definition
{
	el_string name;
	struct el_ast_parameter_list parameter_list;
	struct el_ast_var_type return_type;
	struct el_ast_statement_list code_block;
};

struct el_ast_for_statement
{
	el_string index_var_name;
	el_string value_var_name;
	struct el_ast_expression range;
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

struct el_ast_assignment
{
	struct el_ast_expression lhs;
	struct el_ast_expression rhs;
};

struct el_ast_return_statement
{
	struct el_ast_expression expression;
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
		struct el_ast_return_statement return_statement;
		struct el_ast_expression expression;
	};
};

struct el_ast
{
	struct el_linear_allocator allocator;
	struct el_ast_statement_list root;
};

void el_ast_print(struct el_ast * ast);

// Delete the ast and its decendant nodes
void el_ast_delete(struct el_ast * ast);
