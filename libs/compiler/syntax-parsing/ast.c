#include "ast.h"
#include <containers/string.h>
#include <stdio.h>

static void el_ast_print_indent(int indent)
{
	for(int i = 0; i < indent; ++i)
	{
		printf(" ");
	}
}

static void el_ast_print_statement_list(struct el_ast_statement_list * list, int indent)
{
	for(int i = 0; i < list->num_statements; ++i)
	{
		struct el_ast_statement * s = &list->statements[i];
		el_ast_print_indent(indent);
		switch(s->type)
		{
		case el_AST_NODE_DATA_BLOCK:
			printf("dat %s\n", s->data_block.name);
			for(int j = 0; j < s->data_block.num_var_declarations; ++j)
			{
				el_ast_print_indent(indent + 1);
				struct el_ast_var_decl * var_decl = &s->data_block.var_declarations[j];
				if(var_decl->type.is_native)
				{
					printf("%s %d\n", var_decl->name, var_decl->type.native_type);
				}
				else
				{
					printf("%s %s\n", var_decl->name, var_decl->type.custom_type);
				}
			}
			break;
		case el_AST_NODE_FUNCTION_DEFINITION:
			printf("fnc %s", s->function_definition.name);
			struct el_ast_var_type * type = &s->function_definition.return_type;
			if(type->is_native)
			{
				// Native type NONE is used to represent no return type
				if(type->native_type != el_NONE)
				{
					printf(" -> %d", type->native_type);
				}
			}
			else
			{
				printf(" -> %s", type->custom_type);
			}

			for(int j = 0; j < type->num_dimensions; ++j)
			{
				printf("[]");
			}
			printf("\n");
			el_ast_print_statement_list(&s->function_definition.code_block, indent + 1);
			break;
		case el_AST_NODE_FOR_STATEMENT:
			printf("for %s %s in %s\n", s->for_statement.index_var_name, s->for_statement.value_var_name, s->for_statement.list_name);
			el_ast_print_statement_list(&s->for_statement.code_block, indent + 1);
			break;
		case el_AST_NODE_IF_STATEMENT:
			printf("if %d\n", s->if_statement.expression.todo);
			el_ast_print_statement_list(&s->if_statement.code_block, indent + 1);
			for(int j = 0; j < s->if_statement.num_elif_statements; ++j)
			{
				el_ast_print_indent(indent);
				printf("elif %d\n", s->if_statement.elif_statements[j].expression.todo);
				el_ast_print_statement_list(&s->if_statement.elif_statements[j].code_block, indent + 1);
			}
			if(s->if_statement.else_statement)
			{
				el_ast_print_indent(indent);
				printf("else\n");
				el_ast_print_statement_list(s->if_statement.else_statement, indent + 1);
			}
			break;
		case el_AST_NODE_ASSIGNMENT:
			printf("=\n");
			break;
		}
		printf("\n");
	}
}

void el_ast_print(struct el_ast * ast)
{
	printf("\n\n===\nAST\n===\n\n");
	el_ast_print_statement_list(&ast->root, 0);
}

static void el_ast_var_type_delete(struct el_ast_var_type * t)
{
	if(!t->is_native)
	{
		el_string_delete(t->custom_type);
	}
}

static void el_ast_expression_delete(struct el_ast_expression * e)
{
	// TODO
}

static void el_ast_statement_list_delete(struct el_ast_statement_list * list);

static void el_ast_statement_delete(struct el_ast_statement * s)
{
	switch(s->type)
	{
	case el_AST_NODE_DATA_BLOCK:
		el_string_delete(s->data_block.name);
		for(int i = 0; i < s->data_block.num_var_declarations; ++i)
		{
			struct el_ast_var_decl * var_decl = &s->data_block.var_declarations[i];
			el_string_delete(var_decl->name);
			el_ast_var_type_delete(&var_decl->type);
		}
		ffree(s->data_block.var_declarations);
		break;
	case el_AST_NODE_FUNCTION_DEFINITION:
		el_string_delete(s->function_definition.name);
		el_ast_var_type_delete(&s->function_definition.return_type);
		el_ast_statement_list_delete(&s->function_definition.code_block);
		break;
	case el_AST_NODE_FOR_STATEMENT:
		el_string_delete(s->for_statement.index_var_name);
		el_string_delete(s->for_statement.value_var_name);
		el_string_delete(s->for_statement.list_name);
		el_ast_statement_list_delete(&s->for_statement.code_block);
		break;
	case el_AST_NODE_IF_STATEMENT:
		el_ast_expression_delete(&s->if_statement.expression);
		el_ast_statement_delete(&s->if_statement.code_block);
		el_ast_statement_delete(s->if_statement.else_statement);
		ffree(s->if_statement.else_statement);
		for(int i = 0; i < s->if_statement.num_elif_statements; ++i)
		{
			el_ast_expression_delete(&s->if_statement.elif_statements[i]);
			el_ast_statement_list_delete(&s->if_statement.elif_statements[i].code_block);
		}
		ffree(s->if_statement.elif_statements);
		break;
	case el_AST_NODE_ASSIGNMENT:
		// TODO
		break;
	}
}

static void el_ast_statement_list_delete(struct el_ast_statement_list * list)
{
	if(list)
	{
		for(int i = 0; i < list->num_statements; ++i)
		{
			el_ast_statement_delete(&list->statements[i]);
		}

		ffree(list->statements);
	}
}

void el_ast_delete(struct el_ast * ast)
{
	if(ast)
	{
		el_ast_statement_list_delete(&ast->root);
	}
}
