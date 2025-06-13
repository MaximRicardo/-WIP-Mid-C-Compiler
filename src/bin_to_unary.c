#include "bin_to_unary.h"
#include "identifier.h"
#include "prim_type.h"
#include "safe_mem.h"
#include "token.h"
#include <stdio.h>

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
            (token_tbl->elems[i-1].type != TokenType_R_PAREN &&
            !Token_is_literal(token_tbl->elems[i-1].type) &&
            (token_tbl->elems[i-1].type != TokenType_IDENT ||
             Ident_type_spec(prev_token_src) != PrimType_INVALID));

        m_free(prev_token_src);

        if (!should_convert)
            continue;
    
        token_tbl->elems[i].type =
            Token_convert_to_unary(token_tbl->elems[i].type);

    }

}
