#pragma once

enum el_error
{
	el_SUCCESS,
	el_ALLOCATION_ERROR,

	// Lexing errors
	el_EXCEEDED_TOKENS_LIMIT_LEX_ERROR = 1000,
	el_EXCEEDED_TOKEN_LENGTH_LIMIT_LEX_ERROR,

	// Parsing errors
	el_MATCH_TOKEN_PARSE_ERROR = 2000,
	el_EXPECTED_FACTOR_EXPR_PARSE_ERROR,
	el_EXPECTED_TYPE_PARSE_ERROR
};
