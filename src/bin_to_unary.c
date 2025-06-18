#include "bin_to_unary.h"
#include "safe_mem.h"
#include "token.h"
#include <stdio.h>

static bool is_typecast(const struct TokenList *token_tbl, u32 r_paren_idx) {

    bool ret;
    u32 n_asterisks = 0;

    if (r_paren_idx < 2)
        return false;

    while (token_tbl->elems[r_paren_idx-n_asterisks-1].type == TokenType_MUL ||
            token_tbl->elems[r_paren_idx-n_asterisks-1].type ==
            TokenType_DEREFERENCE)
        ++n_asterisks;

    ret = token_tbl->elems[r_paren_idx-n_asterisks-1].type == TokenType_IDENT &&
            token_tbl->elems[r_paren_idx-n_asterisks-2].type == TokenType_L_PAREN;

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

        should_convert = i == 0 ||
            ((token_tbl->elems[i-1].type != TokenType_R_PAREN ||
              is_typecast(token_tbl, i-1)) &&
            !Token_is_literal(token_tbl->elems[i-1].type) &&
            token_tbl->elems[i-1].type != TokenType_IDENT);

        m_free(prev_token_src);

        if (!should_convert)
            continue;
    
        token_tbl->elems[i].type =
            Token_convert_to_unary(token_tbl->elems[i].type);

    }

}
