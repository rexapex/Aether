#include "symbol.h"
#include <compiler/error.h>
#include <allocators/linear-allocator.h>
#include <stdio.h>
#include <assert.h>

#if 0
	#define DEBUG_RESOLVE(x) do { printf("%s\n", x); } while(0);
#else
	#define DEBUG_RESOLVE(x) do {} while(0);
#endif

#define MAX_NUM_SYMBOLS_PER_SCOPE (16)
#define MAX_NUM_SUB_SCOPES_PER_SCOPE (32)

static struct el_symbol_type INT_SYMBOL_TYPE = {
	.type = 0,
	.num_dimensions = 0,
	.native_type = el_INT_TYPE,
};

static struct el_symbol_type STRING_SYMBOL_TYPE = {
	.type = 0,
	.num_dimensions = 0,
	.native_type = el_STRING_TYPE,
};

static struct el_symbol_type BOOL_SYMBOL_TYPE = {
	.type = 0,
	.num_dimensions = 0,
	.native_type = el_BOOL_TYPE,
};

static int el_resolve_statement(struct el_scope * parent, struct el_ast_statement * statement, struct el_linear_allocator * allocator);
static int el_resolve_expression(struct el_scope * parent, struct el_ast_expression * expression, struct el_linear_allocator * allocator);

static bool el_symbol_types_match(struct el_symbol_type * t1, struct el_symbol_type * t2)
{
	if(t1->type != t2->type || t1->num_dimensions != t2->num_dimensions)
		return false;
	switch(t1->type)
	{
	case el_SYMBOL_TYPE_NATIVE:
		return t1->native_type == t2->native_type;
	case el_SYMBOL_TYPE_CUSTOM:
		return t1->custom_type == t2->custom_type;
	case el_SYMBOL_TYPE_FUNC_DEF:
		if(t1->function_type.num_curried_types != t2->function_type.num_curried_types)
			return false;
		for(int i = 0; i < t1->function_type.num_curried_types; ++i)
		{
			if(!el_symbol_types_match(&t1->function_type.curried_types[i], &t2->function_type.curried_types[i]))
				return false;
		}
		return true;
	case el_SYMBOL_TYPE_ARGS:
		if(t1->args_type.num_curried_types != t2->args_type.num_curried_types)
			return false;
		for(int i = 0; i < t1->args_type.num_curried_types; ++i)
		{
			if(!el_symbol_types_match(&t1->args_type.curried_types[i], &t2->args_type.curried_types[i]))
				return false;
		}
		return true;
	case el_SYMBOL_TYPE_STRUCT:
		// Each struct definition has its own symbol type so can be matched based on pointer comparison
		return t1 == t2;
	}
	// NOTE - STRUCT_DEF types never match
	return false;
}

static struct el_scope * el_add_scope(struct el_scope * parent, struct el_linear_allocator * allocator)
{
	struct el_scope * scope = el_linear_alloc(allocator, sizeof *scope);
	if(!scope)
		return NULL;
	scope->parent = parent;
	scope->num_symbols = 0;
	scope->max_num_symbols = MAX_NUM_SYMBOLS_PER_SCOPE;
	scope->symbols = el_linear_alloc(allocator, sizeof(*scope->symbols) * scope->max_num_symbols);
	if(!scope->symbols)
		return NULL;
	return scope;
}

static struct el_symbol * el_lookup_symbol(struct el_scope * scope, el_string name)
{
	for(int i = 0; i < scope->num_symbols; ++i)
	{
		struct el_symbol * symbol = &scope->symbols[i];
		if(el_string_equals(name, symbol->name))
			return symbol;
	}

	if(scope->parent)
		return el_lookup_symbol(scope->parent, name);

	return NULL;
}

static struct el_symbol_type el_symbol_type_from_var_type(struct el_scope * scope, struct el_ast_var_type * var_type)
{
	if(var_type->is_native)
	{
		struct el_symbol_type symbol_type = {
			.type = el_SYMBOL_TYPE_NATIVE,
			.native_type = var_type->native_type,
			.num_dimensions = var_type->num_dimensions
		};
		return symbol_type;
	}
	else
	{
		struct el_symbol_type symbol_type = {
			.type = el_SYMBOL_TYPE_CUSTOM,
			.custom_type = el_lookup_symbol(scope, var_type->custom_type),
			.num_dimensions = var_type->num_dimensions
		};
		assert(symbol_type.custom_type != NULL); // NOTE - This will fail if the type is used before it's defined
		return symbol_type;
	}
}

static struct el_symbol * el_add_symbol(struct el_scope * parent_scope, el_string name, int type, struct el_linear_allocator * allocator)
{
	if(el_lookup_symbol(parent_scope, name))
	{
		// TODO - Should allow name to be shadowed in nested scopes, i.e. only check if it exists in local scope
		fprintf(stderr, "Failed to add symbol %s, name already exists in the current scope", name);
		return NULL;
	}

	struct el_scope * scope = NULL;
	if(type == el_SYMBOL_TYPE_FUNC_DEF || type == el_SYMBOL_TYPE_STRUCT_DEF)
	{
		scope = el_add_scope(parent_scope, allocator);
		if(!scope)
		{
			fprintf(stderr, "Failed to add scope for symbol %s, allocation error", name);
			return NULL;
		}
	}

	if(parent_scope->num_symbols >= parent_scope->max_num_symbols)
	{
		fprintf(stderr, "Failed to add symbol %s, reached the maximum number of symbols in the current scope %d", name, parent_scope->max_num_symbols);
		return NULL;
	}

	DEBUG_RESOLVE(name);
	struct el_symbol * symbol = &parent_scope->symbols[parent_scope->num_symbols++];
	symbol->type.type = type;
	symbol->name = name;
	symbol->scope = scope;
	return symbol;
}

struct el_scope * el_resolve_symbols(struct el_ast * ast)
{
	struct el_scope * root_scope = el_add_scope(NULL, &ast->allocator);
	if(!root_scope)
		return NULL;

	for(int i = 0; i < ast->root.num_statements; ++i)
	{
		int err = el_resolve_statement(root_scope, &ast->root.statements[i], &ast->allocator);
		if(err != el_SUCCESS)
		{
			fprintf(stderr, "Failed to resolve statement %d, err = %d", i, err);
			return NULL;
		}
	}

	return root_scope;
}

static int el_resolve_statement(struct el_scope * parent, struct el_ast_statement * statement, struct el_linear_allocator * allocator)
{
	switch(statement->type)
	{
	case el_AST_NODE_DATA_BLOCK:
	{
		DEBUG_RESOLVE("data block");
		struct el_symbol * symbol = el_add_symbol(parent, statement->data_block.name, el_SYMBOL_TYPE_STRUCT_DEF, allocator);
		if(!symbol)
			return el_ALLOCATION_ERROR;

		struct el_ast_data_block * data_block = &statement->data_block;

		// Link the symbol info to the ast node
		data_block->symbol = symbol;

		if(data_block->num_var_declarations > symbol->scope->max_num_symbols)
		{
			fprintf(stderr, "Failed to add struct %s, struct has %d fields which exceeds the max number of symbols per scope %d",
				data_block->name, data_block->num_var_declarations, symbol->scope->max_num_symbols);
			return el_EXCEEDED_SCOPE_SYMBOL_LIMIT_ANALYSIS_ERROR;
		}

		struct el_struct_type * struct_type = &data_block->symbol->type.struct_type;
		struct_type->num_field_types = data_block->num_var_declarations;
		struct_type->field_types = NULL;
		if(struct_type->num_field_types > 0)
		{
			struct_type->field_types = el_linear_alloc(allocator, sizeof(*struct_type->field_types) * struct_type->num_field_types);
		}

		for(int i = 0; i < data_block->num_var_declarations; ++i)
		{
			struct el_ast_var_decl * var_decl = &data_block->var_declarations[i];
			struct el_symbol_type symbol_type = el_symbol_type_from_var_type(symbol->scope, &var_decl->type);
			struct el_symbol * var_symbol = el_add_symbol(symbol->scope, var_decl->name, symbol_type.type, allocator);
			var_symbol->type = symbol_type;
			// Link the symbol info to the ast node
			var_decl->symbol = var_symbol;
			// Set the symbol type of the field
			struct_type->field_types[i] = symbol_type;
		}
		break;
	}
	case el_AST_NODE_FUNCTION_DEFINITION:
	{
		DEBUG_RESOLVE("function definition");
		struct el_ast_function_definition * fn_def = &statement->function_definition;

		struct el_symbol * symbol = el_add_symbol(parent, fn_def->name, el_SYMBOL_TYPE_FUNC_DEF, allocator);
		if(!symbol)
			return el_ALLOCATION_ERROR;

		struct el_function_type * fn_type = &symbol->type.function_type;

		// Determine the fn's type signature & add the parameters to the fn's scope
		fn_type->num_curried_types = fn_def->parameter_list.num_parameters + 1;
		fn_type->curried_types = el_linear_alloc(allocator, sizeof(*fn_type->curried_types) * fn_type->num_curried_types);
		for(int i = 0; i < fn_def->parameter_list.num_parameters; ++i)
		{
			struct el_ast_var_decl * param = &fn_def->parameter_list.parameters[i];
			fn_type->curried_types[i] = el_symbol_type_from_var_type(symbol->scope, &param->type);
			param->symbol = el_add_symbol(symbol->scope, param->name, fn_type->curried_types[i].type, allocator);
			param->symbol->type = fn_type->curried_types[i];
		}
		fn_type->curried_types[fn_type->num_curried_types - 1] = el_symbol_type_from_var_type(symbol->scope, &fn_def->return_type);

		for(int i = 0; i < fn_def->code_block.num_statements; ++i)
		{
			int err = el_resolve_statement(symbol->scope, &fn_def->code_block.statements[i], allocator);
			if(err != el_SUCCESS)
				return err;
		}

		// Link the symbol info to the ast node
		fn_def->symbol = symbol;

		break;
	}
	case el_AST_NODE_IF_STATEMENT:
	{
		DEBUG_RESOLVE("if statement");
		// TODO - The other bits
		int err = el_resolve_expression(parent, &statement->if_statement.expression, allocator);
		if(err != el_SUCCESS)
			return err;

		break;
	}
	case el_AST_NODE_ASSIGNMENT:
	{
		DEBUG_RESOLVE("assignment");
		assert(statement->assignment.lhs.type == el_AST_EXPR_IDENTIFIER);
		struct el_symbol * symbol = el_add_symbol(parent, statement->assignment.lhs.identifier, el_SYMBOL_TYPE_UNKNOWN, allocator);
		assert(symbol != NULL);
		int err = el_resolve_expression(parent, &statement->assignment.rhs, allocator);
		if(err != el_SUCCESS)
			return err;
		symbol->type = *statement->assignment.rhs.symbol_type;
		statement->assignment.lhs.symbol_type = statement->assignment.rhs.symbol_type;
		break;
	}
	case el_AST_NODE_RETURN_STATEMENT:
	{
		DEBUG_RESOLVE("return statement");
		int err = el_resolve_expression(parent, &statement->expression, allocator);
		if(err != el_SUCCESS)
			return err;
		break;
	}
	}
	return el_SUCCESS;
}

static int el_resolve_expression(struct el_scope * parent, struct el_ast_expression * expression, struct el_linear_allocator * allocator)
{
	int err = 0;
	switch(expression->type)
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
	{
		DEBUG_RESOLVE("binary expression");
		err = err || el_resolve_expression(parent, expression->binary_op.lhs, allocator);
		err = err || el_resolve_expression(parent, expression->binary_op.rhs, allocator);
		if(err != el_SUCCESS)
			return err;
		if(!el_symbol_types_match(expression->binary_op.lhs->symbol_type, expression->binary_op.rhs->symbol_type))
			return el_NON_MATCHING_TYPES_ANALYSIS_ERROR;
		if(expression->type >= el_AST_EXPR_EQUALS && expression->type <= el_AST_EXPR_BOOLEAN_OR)
		{
			expression->symbol_type = &BOOL_SYMBOL_TYPE;
		}
		else
		{
			expression->symbol_type = expression->binary_op.lhs->symbol_type;
		}
		break;
	}
	case el_AST_EXPR_DOT:
	{
		DEBUG_RESOLVE("dot expression");
		err = err || el_resolve_expression(parent, expression->binary_op.lhs, allocator);
		struct el_symbol_type * lhs_symbol_type = expression->binary_op.lhs->symbol_type;
		assert(lhs_symbol_type->type == el_SYMBOL_TYPE_CUSTOM);
		err = err || el_resolve_expression(lhs_symbol_type->custom_type->scope, expression->binary_op.rhs, allocator);
		expression->symbol_type = expression->binary_op.rhs->symbol_type;
		break;
	}
	case el_AST_EXPR_FUNCTION_CALL:
	{
		DEBUG_RESOLVE("function call");
		err = err || el_resolve_expression(parent, expression->binary_op.lhs, allocator);
		err = err || el_resolve_expression(parent, expression->binary_op.rhs, allocator);
		// TODO - The function's symbol should be part of the call's type
		struct el_symbol_type * symbol_type = expression->binary_op.lhs->symbol_type;
		assert(symbol_type->type == el_SYMBOL_TYPE_FUNC_DEF);
		struct el_function_type * fn_type = &symbol_type->function_type;
		assert(fn_type->num_curried_types > 0);
		expression->symbol_type = &fn_type->curried_types[fn_type->num_curried_types - 1];
		break;
	}
	case el_AST_EXPR_LIST_INDEX:
	{
		DEBUG_RESOLVE("list index");
		err = err || el_resolve_expression(parent, expression->binary_op.lhs, allocator);
		err = err || el_resolve_expression(parent, expression->binary_op.rhs, allocator);
		if(expression->binary_op.lhs->expression_list->num_expressions > 0)
			expression->symbol_type = expression->binary_op.lhs->expression_list->expressions[0].symbol_type;
		break;
	}
	case el_AST_EXPR_NUMBER_LITERAL:
	{
		DEBUG_RESOLVE("number literal");
		expression->symbol_type = &INT_SYMBOL_TYPE;
		break;
	}
	case el_AST_EXPR_STRING_LITERAL:
	{
		DEBUG_RESOLVE("string literal");
		expression->symbol_type = &STRING_SYMBOL_TYPE;
		break;
	}
	case el_AST_EXPR_LIST_LITERAL:
	{
		DEBUG_RESOLVE("list literal");
		for(int i = 0; i < expression->expression_list->num_expressions; ++i)
		{
			err = err || el_resolve_expression(parent, &expression->expression_list->expressions[i], allocator);
		}

		expression->symbol_type = el_linear_alloc(allocator, sizeof *expression->symbol_type);
		struct el_symbol_type * symbol_type = expression->symbol_type;
		if(expression->expression_list->num_expressions > 0)
		{
			// Use the first element of the list to deduce the list type
			*symbol_type = *expression->expression_list->expressions[0].symbol_type;
			// TODO - Test with multi-dimensional list literals
			symbol_type->num_dimensions++;
		}
		else
		{
			// NOTE - This could be an empty list, which should deduce its element type from context
			symbol_type->type = el_SYMBOL_TYPE_UNKNOWN;
			symbol_type->num_dimensions = 1;
		}
		break;
	}
	case el_AST_EXPR_ARGUMENTS:
	{
		DEBUG_RESOLVE("arguments");
		for(int i = 0; i < expression->expression_list->num_expressions; ++i)
		{
			err = err || el_resolve_expression(parent, &expression->expression_list->expressions[i], allocator);
		}

		expression->symbol_type = el_linear_alloc(allocator, sizeof *expression->symbol_type);
		expression->symbol_type->type = el_SYMBOL_TYPE_ARGS;

		struct el_args_type * args_type = &expression->symbol_type->args_type;
		args_type->num_curried_types = expression->expression_list->num_expressions;
		args_type->curried_types = el_linear_alloc(allocator, sizeof(*args_type->curried_types) * args_type->num_curried_types);
		for(int i = 0; i < args_type->num_curried_types; ++i)
		{
			args_type->curried_types[i] = *expression->expression_list->expressions[i].symbol_type;
		}
		break;
	}
	case el_AST_EXPR_IDENTIFIER:
	{
		DEBUG_RESOLVE("identifier");
		struct el_symbol * symbol = el_lookup_symbol(parent, expression->identifier);
		if(!symbol)
			return el_UNKNOWN_SYMBOL_ANALYSIS_ERROR;
		expression->symbol_type = &symbol->type;
		break;
	}
	}
	return err;
}
