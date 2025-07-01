#include "bin_to_unary.h"
#include "utils/safe_mem.h"
#include "token.h"
#include <stdio.h>

static bool is_typecast(const struct TokenList *token_tbl, u32 r_paren_idx) {

    bool ret;
    u32 asterisks = 0;

    const struct Token *tok = NULL;

    if (r_paren_idx < 2)
        return false;

    while (token_tbl->elems[r_paren_idx-asterisks-1].type == TokenType_MUL ||
            token_tbl->elems[r_paren_idx-asterisks-1].type ==
            TokenType_DEREFERENCE)
        ++asterisks;

    tok = &token_tbl->elems[r_paren_idx-asterisks-1];

    ret = tok->type == TokenType_IDENT || Token_is_type_mod(tok->type);

    return ret;

}

void BinToUnary_convert(struct TokenList *token_tbl) {

    u32 i;

    for (i = 0; i < token_tbl->size; i++) {

        bool should_convert;
        char *prev_token_src = NULL;

        if (!Token_is_bin_operator(token_tbl->elems[i].type) ||
                !Token_has_unary_version(token_tbl->elems[i].type))
            continue;

        prev_token_src = Token_src(&token_tbl->elems[i-1]);

        /* you can deduce whether or not an operator token is a unary or
         * binary one, based on just the preceding token. */
        should_convert = i == 0 ||
            ((token_tbl->elems[i-1].type != TokenType_R_PAREN ||
              is_typecast(token_tbl, i-1)) &&
            !Token_is_literal(token_tbl->elems[i-1].type) &&
            token_tbl->elems[i-1].type != TokenType_IDENT &&
            token_tbl->elems[i-1].type != TokenType_R_ARR_SUBSCR);

        m_free(prev_token_src);

        if (!should_convert)
            continue;
    
        token_tbl->elems[i].type =
            Token_convert_to_unary(token_tbl->elems[i].type);

    }

}
