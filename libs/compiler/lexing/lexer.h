#pragma once
#include "token-stream.h"

// Generate a token stream from a source file
// Streams created with this fn must be deleted by calling el_token_stream_delete
struct el_token_stream el_lexer_lex_file(struct el_text_file * f);
