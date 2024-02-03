#include <stdio.h>
#include <file-system/path.h>
#include <file-system/file-system.h>
#include <compiler/lexing/token-stream.h>
#include <compiler/lexing/lexer.h>
#include <compiler/syntax-parsing/ast.h>
#include <compiler/syntax-parsing/parser.h>

int main(int argc, char const * argv[])
{
	if(argc < 2)
		return 0;

	char const * path = argv[1];
	printf("Compiling %s\n\n", path);

	struct el_text_file text_file = el_text_file_new(path);
	if(!text_file.contents)
		goto close_file;

	struct el_token_stream token_stream = el_lex_file(&text_file);
	if(!token_stream.tokens)
		goto free_token_stream;

	struct el_ast ast = el_parse_token_stream(&token_stream);

	el_ast_delete(&ast);

free_token_stream:
	el_token_stream_delete(&token_stream);

close_file:
	el_text_file_delete(&text_file);

	return 0;
}
