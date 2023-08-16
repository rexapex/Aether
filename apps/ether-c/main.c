#include <stdio.h>
#include <file-system/path.h>
#include <file-system/file-system.h>
#include <compiler/lexing/lexer.h>

int main()
{
	struct el_text_file text_file = el_text_file_new("C:\\Users\\james\\OneDrive\\Documents\\Ether Language\\Example.txt");

	struct el_token_stream token_stream = el_lexer_lex_file(&text_file);

	el_text_file_delete(&text_file);
	el_token_stream_delete(&token_stream);

	return 0;
}
