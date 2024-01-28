#pragma once

enum el_error
{
	el_SUCCESS,
	el_ALLOCATION_ERROR,

	// Lexing errors
	// TODO - Start at 1000

	// Parsing errors
	el_MATCH_TOKEN_PARSE_ERROR = 2000,
	el_EXPECTED_FACTOR_EXPR_PARSE_ERROR,
	el_EXPECTED_TYPE_PARSE_ERROR
};
