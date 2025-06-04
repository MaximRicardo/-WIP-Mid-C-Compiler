#include "lexer.h"
#include "token.h"
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

struct Lexer Lexer_init(void) {

    struct Lexer lexer;
    lexer.error = false;
    lexer.token_tbl = TokenList_init();
    return lexer;

}

struct Lexer Lexer_create(bool error, struct TokenList token_tbl) {

    struct Lexer lexer;
    lexer.error = error;
    lexer.token_tbl = token_tbl;
    return lexer;

}

void Lexer_free(struct Lexer *lexer) {

    TokenList_free(&lexer->token_tbl);

}

struct Lexer Lexer_lex(const char *src) {

    bool error = false;
    struct TokenList token_tbl = TokenList_init();
    u32 src_i;
    unsigned line_num=1, column_num=1;

    for (src_i = 0; src[src_i] != '\0'; src_i++,column_num++) {

        if (isspace(src[src_i])) {
            /* Do nothing */
        }
        else if (src[src_i] == '\n') {
            ++line_num;
            column_num = 0;
        }
        else if (src[src_i] == '/' && src[src_i+1] == '/') {
            ++line_num;
            column_num = 0;
            while (src[++src_i] != '\n');
        }
        else if (src[src_i] == '/' && src[src_i+1] == '*') {
            while (src[src_i] != '*' || src[src_i+1] != '/') {
                if (src[src_i] == '\n') {
                    ++line_num;
                    column_num = 0;
                }
                ++src_i;
            }
        }

        else if (src[src_i] == '+')
            TokenList_push_back(&token_tbl, Token_create(line_num, column_num,
                        TokenType_PLUS));
        else if (src[src_i] == '-')
            TokenList_push_back(&token_tbl, Token_create(line_num, column_num,
                        TokenType_MINUS));
        else if (src[src_i] == '*')
            TokenList_push_back(&token_tbl, Token_create(line_num, column_num,
                        TokenType_MUL));
        else if (src[src_i] == '/')
            TokenList_push_back(&token_tbl, Token_create(line_num, column_num,
                        TokenType_DIV));

        else if (isdigit(src[src_i])) {
            /* An integer literal */
            char *end_ptr = NULL;
            u32 chars_moved;
            union TokenValue value;
            value.int_value = strtoul(&src[src_i], &end_ptr, 0);
            TokenList_push_back(&token_tbl, Token_create_w_val(line_num,
                        column_num, TokenType_INT_LIT, value));
            chars_moved = end_ptr-&src[src_i];
            src_i += chars_moved-1;
            column_num += chars_moved-1;
        }

    }

    return Lexer_create(error!=false, token_tbl);

}
