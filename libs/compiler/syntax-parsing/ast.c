#include "ast.h"
#include <containers/string.h>
#include <stdio.h>

static char const * expr_names[] = {
	"=",
	">",
	"<",
	">=",
	"<=",

	"&&",
	"||",

	"+",
	"-",
	"*",
	"/",

	".",

	"call",
	"index",

	"N/A",
	"N/A",
	"slice",

	"args",

	"N/A",
};

static int indent_incr = 2;

static void el_ast_print_indent(int indent)
{
	for(int i = 0; i < indent; ++i)
	{
		printf(" ");
	}
}

static void el_ast_print_var_type(struct el_ast_var_type * t)
{
	if(t->is_native)
	{
		switch(t->native_type)
		{
		case el_NONE:
			printf("void");
			break;
		case el_INT_TYPE:
			printf("int");
			break;
		case el_FLOAT_TYPE:
			printf("float");
			break;
		default:
			assert(false);
			break;
		}
	}
	else
	{
		printf(t->custom_type);
	}
}

static void el_ast_print_expr(struct el_ast_expression * e, int indent);

static void el_ast_print_expr_list(struct el_ast_expression_list * l, int indent)
{
	for(int i = 0; i < l->num_expressions; ++i)
	{
		el_ast_print_expr(&l->expressions[i], indent);
	}
}

static void el_ast_print_expr(struct el_ast_expression * e, int indent)
{
	el_ast_print_indent(indent);
	switch(e->type)
	{
	case el_AST_EXPR_EQUALS:
	case el_AST_EXPR_GREATER_THAN:
	case el_AST_EXPR_LESS_THAN:
	case el_AST_EXPR_GEQUALS:
	case el_AST_EXPR_LEQUALS:
	case el_AST_EXPR_BOOLEAN_AND:
	case el_AST_EXPR_BOOLEAN_OR:
	case el_AST_EXPR_ADD:
	case el_AST_EXPR_SUB:
	case el_AST_EXPR_MUL:
	case el_AST_EXPR_DIV:
	case el_AST_EXPR_DOT:
	case el_AST_EXPR_FUNCTION_CALL:
	case el_AST_EXPR_SLICE_INDEX:
		printf("%s\n", expr_names[e->type]);
		el_ast_print_expr(e->binary_op.lhs, indent + indent_incr);
		el_ast_print_expr(e->binary_op.rhs, indent + indent_incr);
		break;
	case el_AST_EXPR_ARGUMENTS:
	case el_AST_EXPR_SLICE_LITERAL:
		printf("%s\n", expr_names[e->type]);
		el_ast_print_expr_list(e->expression_list, indent + indent_incr);
		break;
	case el_AST_EXPR_NUMBER_LITERAL:
		printf(e->number_literal);
		break;
	case el_AST_EXPR_STRING_LITERAL:
		printf(e->string_literal);
		break;
	case el_AST_EXPR_IDENTIFIER:
		printf(e->identifier);
		break;
	}
	printf("\n");
}

static void el_ast_print_statement_list(struct el_ast_statement_list * list, int indent)
{
	for(int i = 0; i < list->num_statements; ++i)
	{
		struct el_ast_statement * s = &list->statements[i];
		switch(s->type)
		{
		case el_AST_NODE_DATA_BLOCK:
			el_ast_print_indent(indent);
			printf("dat %s\n", s->data_block.name);
			for(int j = 0; j < s->data_block.num_var_declarations; ++j)
			{
				el_ast_print_indent(indent + indent_incr);
				struct el_ast_var_decl * var_decl = &s->data_block.var_declarations[j];
				el_ast_print_var_type(&var_decl->type);
				printf(" %s\n", var_decl->name);
			}
			break;
		case el_AST_NODE_FUNCTION_DEFINITION:
			el_ast_print_indent(indent);
			printf("fnc %s : ", s->function_definition.name);

			for(int j = 0; j < s->function_definition.parameter_list.num_parameters; ++j)
			{
				struct el_ast_var_decl * var_decl = &s->function_definition.parameter_list.parameters[j];
				printf("(");
				el_ast_print_var_type(&var_decl->type);
				printf(" %s) -> ", var_decl->name);
			}

			struct el_ast_var_type * type = &s->function_definition.return_type;
			el_ast_print_var_type(type);

			for(int j = 0; j < type->num_dimensions; ++j)
			{
				printf("[]");
			}
			printf("\n");
			el_ast_print_statement_list(&s->function_definition.code_block, indent + indent_incr);
			break;
		case el_AST_NODE_FOR_STATEMENT:
		case el_AST_NODE_IF_STATEMENT:
			assert(false);
			break;
		case el_AST_NODE_ASSIGNMENT:
			el_ast_print_indent(indent);
			printf("=\n");
			el_ast_print_expr(&s->assignment.lhs, indent + indent_incr);
			el_ast_print_expr(&s->assignment.rhs, indent + indent_incr);
			break;
		case el_AST_NODE_RETURN_STATEMENT:
			el_ast_print_indent(indent);
			printf("return\n");
			el_ast_print_expr(&s->return_statement.expression, indent + indent_incr);
			break;
		case el_AST_NODE_EXPRESSION:
			el_ast_print_expr(&s->expression, indent);
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

static void el_ast_expression_delete(struct el_ast_expression * e);

static void el_ast_expression_list_delete(struct el_ast_expression_list * l)
{
	for(int i = 0; i < l->num_expressions; ++i)
	{
		el_ast_expression_delete(&l->expressions[i]);
	}
	ffree(l->expressions);
	free(l);
}

static void el_ast_expression_delete(struct el_ast_expression * e)
{
	if(e)
	{
		switch(e->type)
		{
		case el_AST_EXPR_EQUALS:
		case el_AST_EXPR_GREATER_THAN:
		case el_AST_EXPR_LESS_THAN:
		case el_AST_EXPR_GEQUALS:
		case el_AST_EXPR_LEQUALS:
		case el_AST_EXPR_BOOLEAN_AND:
		case el_AST_EXPR_BOOLEAN_OR:
		case el_AST_EXPR_ADD:
		case el_AST_EXPR_SUB:
		case el_AST_EXPR_MUL:
		case el_AST_EXPR_DIV:
		case el_AST_EXPR_DOT:
		case el_AST_EXPR_FUNCTION_CALL:
		case el_AST_EXPR_SLICE_INDEX:
			el_ast_expression_delete(e->binary_op.lhs);
			el_ast_expression_delete(e->binary_op.rhs);
			ffree(e->binary_op.lhs);
			ffree(e->binary_op.rhs);
			break;
		case el_AST_EXPR_NUMBER_LITERAL:
			el_string_delete(e->number_literal);
			break;
		case el_AST_EXPR_STRING_LITERAL:
			el_string_delete(e->number_literal);
			break;
		case el_AST_EXPR_SLICE_LITERAL:
		case el_AST_EXPR_ARGUMENTS:
			el_ast_expression_list_delete(e->expression_list);
			break;
		case el_AST_EXPR_IDENTIFIER:
			el_string_delete(e->identifier);
			break;
		default:
			assert(false);
			break;
		}
	}
}

static void el_ast_parameter_list_delete(struct el_ast_parameter_list * l)
{
	for(int i = 0; i < l->num_parameters; ++i)
	{
		el_string_delete(l->parameters[i].name);
		el_ast_var_type_delete(&l->parameters[i].type);
	}
	ffree(l->parameters);
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
		el_ast_parameter_list_delete(&s->function_definition.parameter_list);
		break;
	case el_AST_NODE_FOR_STATEMENT:
		el_string_delete(s->for_statement.index_var_name);
		el_string_delete(s->for_statement.value_var_name);
		el_ast_expression_delete(&s->for_statement.range);
		el_ast_statement_list_delete(&s->for_statement.code_block);
		break;
	case el_AST_NODE_IF_STATEMENT:
		el_ast_expression_delete(&s->if_statement.expression);
		el_ast_statement_list_delete(&s->if_statement.code_block);
		el_ast_statement_list_delete(s->if_statement.else_statement);
		ffree(s->if_statement.else_statement);
		for(int i = 0; i < s->if_statement.num_elif_statements; ++i)
		{
			el_ast_expression_delete(&s->if_statement.elif_statements[i].expression);
			el_ast_statement_list_delete(&s->if_statement.elif_statements[i].code_block);
		}
		ffree(s->if_statement.elif_statements);
		break;
	case el_AST_NODE_ASSIGNMENT:
		el_ast_expression_delete(&s->assignment.lhs);
		el_ast_expression_delete(&s->assignment.rhs);
		break;
	case el_AST_NODE_RETURN_STATEMENT:
		el_ast_expression_delete(&s->return_statement.expression);
		break;
	case el_AST_NODE_EXPRESSION:
		el_ast_expression_delete(&s->expression);
		break;
	default:
		assert(false);
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
