#include "ast.h"
#include <containers/string.h>

void el_ast_delete_node(struct el_ast_node * node)
{
	if(node)
	{
		switch(node->type)
		{
		case el_LIST:
			for(int i = 0; i < node->list.num_nodes; ++i)
			{
				el_ast_delete_node(node->list.nodes + i);
			}
			break;
		case el_BINARY_OP:
			el_ast_delete_node(node->binary_op.left);
			el_ast_delete_node(node->binary_op.right);
			break;
		case el_LITERAL:
			el_string_delete(node->literal.value);
			break;
		}

		if(node->token.source)
		{
			el_string_delete(node->token.source);
		}

		ffree(node);
	}
}

void el_ast_delete(struct el_ast * ast)
{
	if(ast)
	{
		el_ast_delete_node(ast->root);
	}
}
