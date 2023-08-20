#pragma once
#include "ast.h"

struct el_token_stream;

struct el_ast el_parse_token_stream(struct el_token_stream * token_stream);
