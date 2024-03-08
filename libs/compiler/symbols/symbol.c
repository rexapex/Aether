#include "symbol.h"
#include <allocators/linear-allocator.h>
#include <stdio.h>
#include <assert.h>

#define MAX_NUM_SYMBOLS_PER_SCOPE (16)
#define MAX_NUM_SUB_SCOPES_PER_SCOPE (32)

static struct el_scope * el_add_scope(struct el_scope * parent, struct el_linear_allocator * allocator)
{
	struct el_scope * scope = el_linear_alloc(allocator, sizeof(struct el_scope));
	if(!scope)
		return NULL;
	scope->parent = parent;
	scope->num_symbols = 0;
	scope->max_num_symbols = MAX_NUM_SYMBOLS_PER_SCOPE;
	scope->symbols = el_linear_alloc(allocator, sizeof(struct el_symbol) * scope->max_num_symbols);
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
		assert(symbol_type.custom_type); // NOTE - This will fail if the type is used before it's defined
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
		struct el_ast_statement * statement = &ast->root.statements[i];
		switch(statement->type)
		{
		case el_AST_NODE_DATA_BLOCK:
		{
			struct el_symbol * symbol = el_add_symbol(root_scope, statement->data_block.name, el_SYMBOL_TYPE_STRUCT_DEF, &ast->allocator);
			if(!symbol)
				return NULL;

			// Link the symbol info to the ast node
			statement->data_block.symbol = symbol;

			if(statement->data_block.num_var_declarations > symbol->scope->max_num_symbols)
			{
				fprintf(stderr, "Failed to add struct %s, struct has %d fields which exceeds the max number of symbols per scope %d",
					statement->data_block.name, statement->data_block.num_var_declarations, symbol->scope->max_num_symbols);
				return NULL;
			}

			for(int i = 0; i < statement->data_block.num_var_declarations; ++i)
			{
				struct el_ast_var_decl * var_decl = &statement->data_block.var_declarations[i];
				struct el_symbol_type symbol_type = el_symbol_type_from_var_type(symbol->scope, &var_decl->type);
				struct el_symbol * var_symbol = el_add_symbol(symbol->scope, var_decl->name, symbol_type.type, &ast->allocator);
				var_symbol->type = symbol_type;
				// Link the symbol info to the ast node
				var_decl->symbol = var_symbol;
			}
			break;
		}
		case el_AST_NODE_FUNCTION_DEFINITION:
		{
			struct el_symbol * symbol = el_add_symbol(root_scope, statement->function_definition.name, el_SYMBOL_TYPE_FUNC_DEF, &ast->allocator);
			if(!symbol)
				return NULL;

			// Link the symbol info to the ast node
			statement->function_definition.symbol = symbol;

			break;
		}
		}
	}

	return root_scope;
}
