#include "pre_to_post_fix.h"
#include "token.h"
#include <assert.h>
#include <stdio.h>

static bool convertable(enum TokenType type) {

    return type == TokenType_PREFIX_INC || type == TokenType_PREFIX_DEC;

}

static enum TokenType convert(enum TokenType type) {

    switch (type) {

    case TokenType_PREFIX_INC:
        return TokenType_POSTFIX_INC;

    case TokenType_PREFIX_DEC:
        return TokenType_POSTFIX_DEC;

    default:
        assert(false);

    }

}

void PreToPostFix_convert(struct TokenList *token_tbl) {

    u32 i;

    for (i = 0; i < token_tbl->size; i++) {

        if (!convertable(token_tbl->elems[i].type))
            continue;

        if (i > 0 && (token_tbl->elems[i-1].type == TokenType_IDENT ||
                    token_tbl->elems[i-1].type == TokenType_R_PAREN ||
                    Token_is_literal(token_tbl->elems[i-1].type)))
            token_tbl->elems[i].type = convert(token_tbl->elems[i].type);

    }

}
