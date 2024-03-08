#include "analyser.h"
#include <compiler/symbols/symbol.h>

void el_analyse_ast(struct el_ast * ast)
{
	struct el_scope * global_scope = el_resolve_symbols(ast);
	(void)global_scope;
}
