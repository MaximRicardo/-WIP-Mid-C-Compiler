#pragma once

#include "ast.h"
#include "bool.h"
#include "token.h"

extern bool SY_error_occurred;

/*
 * stop_type    - Stop reading tokens after encountering a token of type
 *                stop_type.
 * function automatically stops at if it reaches the end of the token table.
 * end_idx, if not NULL, is set to the index of the first token after the
 * expression, i.e. either the first token of type stop_type or
 * token_tbl->size.
 */
struct Expr* SY_shunting_yard(const struct TokenList *token_tbl, u32 start_idx,
        enum TokenType stop_type, u32 *end_idx);
