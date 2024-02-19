#pragma once

struct el_text_file;

// Generate a token stream from a source file
// Streams created with this fn must be deleted by calling el_token_stream_delete
struct el_token_stream el_lex_file(struct el_text_file * f);
