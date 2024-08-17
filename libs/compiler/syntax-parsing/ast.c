#include "ast.h"
#include <compiler/lexing/token-stream.h>
#include <compiler/symbols/symbol.h>
#include <allocators/fmalloc.h>
#include <containers/string.h>
#include <stdio.h>
#include <assert.h>

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
	"list",

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
		case el_FLOAT_TYPE:
		case el_STRING_TYPE:
		case el_BOOL_TYPE:
			printf("%s", token_strings[t->native_type]);
			break;
		default:
			assert(false);
			break;
		}
	}
	else
	{
		printf("%s", t->custom_type);
	}

	for(int j = 0; j < t->num_dimensions; ++j)
	{
		printf("[]");
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

static void el_ast_print_name_and_type(char const * name, struct el_symbol_type * symbol_type, char delimiter)
{
	printf("%s", name);
	if(symbol_type)
	{
		printf(" {");
		switch(symbol_type->type)
		{
		case el_SYMBOL_TYPE_NATIVE:
		{
			printf("%s", token_strings[symbol_type->native_type]);
			break;
		}
		case el_SYMBOL_TYPE_CUSTOM:
		{
			printf("%s", symbol_type->custom_type->name);
			break;
		}
		case el_SYMBOL_TYPE_FUNC_DEF:
		{
			printf("fn %d", symbol_type->function_type.num_curried_types);
			break;
		}
		case el_SYMBOL_TYPE_STRUCT_DEF:
		{
			printf("struct");
			struct el_struct_type * struct_type = &symbol_type->struct_type;
			for(int i = 0; i < struct_type->num_field_types; ++i)
			{
				el_ast_print_name_and_type("", &struct_type->field_types[i], '\0');
			}
			break;
		}
		case el_SYMBOL_TYPE_ARGS:
		{
			printf("args %d", symbol_type->args_type.num_curried_types);
			break;
		}
		case el_SYMBOL_TYPE_UNKNOWN:
		{
			printf("?");
			break;
		}
		}
		for(int i = 0; i < symbol_type->num_dimensions; ++i)
		{
			printf("[]");
		}
		printf("}");
	}
	printf("%c", delimiter);
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
	case el_AST_EXPR_LIST_INDEX:
		el_ast_print_name_and_type(expr_names[e->type], e->symbol_type, '\n');
		el_ast_print_expr(e->binary_op.lhs, indent + indent_incr);
		el_ast_print_expr(e->binary_op.rhs, indent + indent_incr);
		break;
	case el_AST_EXPR_ARGUMENTS:
	case el_AST_EXPR_LIST_LITERAL:
		el_ast_print_name_and_type(expr_names[e->type], e->symbol_type, '\n');
		el_ast_print_expr_list(e->expression_list, indent + indent_incr);
		break;
	case el_AST_EXPR_NUMBER_LITERAL:
		el_ast_print_name_and_type(e->number_literal, e->symbol_type, '\n');
		break;
	case el_AST_EXPR_STRING_LITERAL:
		el_ast_print_name_and_type(e->string_literal, e->symbol_type, '\n');
		break;
	case el_AST_EXPR_IDENTIFIER:
		el_ast_print_name_and_type(e->identifier, e->symbol_type, '\n');
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
			el_ast_print_name_and_type(s->data_block.name, &s->data_block.symbol->type, '\n');
			for(int j = 0; j < s->data_block.num_var_declarations; ++j)
			{
				el_ast_print_indent(indent + indent_incr);
				struct el_ast_var_decl * var_decl = &s->data_block.var_declarations[j];
				el_ast_print_name_and_type(var_decl->name, &var_decl->symbol->type, '\n');
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
	assert(ast);
	printf("\n\n===\nAST\n===\n\n");
	el_ast_print_statement_list(&ast->root, 0);
}

void el_ast_delete(struct el_ast * ast)
{
	if(ast)
	{
		ffree(ast->allocator.memory);
		ast->allocator.memory = NULL;
		ast->allocator.capacity = 0;
		ast->allocator.size = 0;
	}
}
