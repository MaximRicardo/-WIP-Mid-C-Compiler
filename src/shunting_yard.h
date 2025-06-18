#pragma once

#include "ast.h"
#include "bool.h"
#include "token.h"
#include "parser_var.h"
#include "typedef.h"

extern bool SY_error_occurred;

/*
 * this function automatically stops at the first semicolon it finds,
 * regardless of whether or not it's in parentheses.
 * stop_types   - Stop reading tokens after encountering a token with a type in
 *                stop_types. Doesn't stop if that token is inside another
 *                of parentheses.
 * function automatically stops at if it reaches the end of the token table.
 * end_idx, if not NULL, is set to the index of the first token after the
 * expression, i.e. either the first token of type stop_type or
 * token_tbl->size.
 */
struct Expr* SY_shunting_yard(const struct TokenList *token_tbl, u32 start_idx,
        enum TokenType *stop_types, u32 n_stop_types, u32 *end_idx,
        const struct ParVarList *vars, u32 bp, bool is_initializer,
        const struct TypedefList *typedefs);
