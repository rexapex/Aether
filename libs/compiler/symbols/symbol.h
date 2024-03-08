#pragma once
#include <compiler/syntax-parsing/ast.h>

struct el_linear_allocator;

enum el_symbol_type_type
{
	el_SYMBOL_TYPE_NATIVE,
	el_SYMBOL_TYPE_CUSTOM,
	el_SYMBOL_TYPE_FUNC_DEF,
	el_SYMBOL_TYPE_STRUCT_DEF
};

struct el_symbol_type
{
	union
	{
		int native_type;
		struct el_symbol * custom_type;
	};
	int num_dimensions; // 0 for single, 1 for list, 2 for 2-dimensional list, etc.
	int type;
};

struct el_symbol
{
	// The scope defined by the symbol, or NULL if the symbol does not create a scope
	struct el_scope * scope;
	struct el_symbol_type type;
	el_string name;
};

struct el_scope
{
	struct el_scope * parent;
	struct el_symbol * symbols;
	int max_num_symbols;
	int num_symbols;
};

struct el_scope * el_resolve_symbols(struct el_ast * ast);
