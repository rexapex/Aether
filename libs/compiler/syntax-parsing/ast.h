#pragma once
#include <compiler/lexing/token-stream.h>

enum el_node_type
{
	el_LIST,
	el_BINARY_OP,
	el_LITERAL
};

struct el_ast_node
{
	int type;
	struct el_token token;

	union
	{
		struct
		{
			struct el_ast_node * nodes;
			int num_nodes;
		} list;

		struct
		{
			struct el_ast_node * left;
			struct el_ast_node * right;
		} binary_op;

		struct
		{
			el_string value;
		} literal;
	};
};

struct el_ast
{
	struct el_ast_node * root;
};

void el_ast_delete_node(struct el_ast_node * node);

// Delete the ast and its decendant nodes
void el_ast_delete(struct el_ast * ast);
