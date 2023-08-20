#include <stdio.h>
#include <file-system/path.h>
#include <file-system/file-system.h>
#include <compiler/lexing/token-stream.h>
#include <compiler/lexing/lexer.h>
#include <compiler/syntax-parsing/ast.h>
#include <compiler/syntax-parsing/parser.h>

int main()
{
	struct el_text_file text_file = el_text_file_new("C:\\Users\\james\\OneDrive\\Documents\\Ether Language\\Example.txt");

	struct el_token_stream token_stream = el_lex_file(&text_file);

	struct el_ast ast = el_parse_token_stream(&token_stream);

	el_ast_delete(&ast);
	el_token_stream_delete(&token_stream);
	el_text_file_delete(&text_file);

	return 0;
}
