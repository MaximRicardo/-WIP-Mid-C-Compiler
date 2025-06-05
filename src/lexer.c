#include "lexer.h"
#include "safe_mem.h"
#include "token.h"
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

bool Lexer_error_occurred = false;

struct Lexer Lexer_init(void) {

    struct Lexer lexer;
    lexer.token_tbl = TokenList_init();
    return lexer;

}

struct Lexer Lexer_create(struct TokenList token_tbl) {

    struct Lexer lexer;
    lexer.token_tbl = token_tbl;
    return lexer;

}

void Lexer_free(struct Lexer *lexer) {

    TokenList_free(&lexer->token_tbl);

}

static bool valid_ident_start_char(char c) {

    return isalpha(c) || c == '_';

}

static bool valid_ident_char(char c) {

    return isalnum(c) || c == '_';

}

static unsigned get_identifier_len(const char *ident_start) {

    unsigned i;

    assert(valid_ident_start_char(ident_start[0]));

    for (i = 1; ident_start[i] != '\0'; i++) {
        if (!valid_ident_char(ident_start[i]))
            break;
    }

    return i;

}

struct Lexer Lexer_lex(const char *src) {

    struct TokenList token_tbl = TokenList_init();
    u32 src_i;
    unsigned line_num=1, column_num=1;

    Lexer_error_occurred = false;

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

        else if (src[src_i] == ';')
            TokenList_push_back(&token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, TokenType_SEMICOLON));

        else if (src[src_i] == '+')
            TokenList_push_back(&token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, TokenType_PLUS));
        else if (src[src_i] == '-')
            TokenList_push_back(&token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, TokenType_MINUS));
        else if (src[src_i] == '*')
            TokenList_push_back(&token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, TokenType_MUL));
        else if (src[src_i] == '/')
            TokenList_push_back(&token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, TokenType_DIV));

        else if (src[src_i] == '(')
            TokenList_push_back(&token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, TokenType_L_PAREN));
        else if (src[src_i] == ')')
            TokenList_push_back(&token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, TokenType_R_PAREN));

        else if (isdigit(src[src_i])) {
            /* An integer literal */
            char *end_ptr = NULL;
            u32 chars_moved;
            union TokenValue value;
            value.int_value = strtoul(&src[src_i], &end_ptr, 0);
            chars_moved = end_ptr-&src[src_i];
            TokenList_push_back(&token_tbl, Token_create_w_val(line_num,
                        column_num, &src[src_i], chars_moved,
                        TokenType_INT_LIT, value));
            src_i += chars_moved-1;
            column_num += chars_moved-1;
        }

        else {
            bool is_identifier = valid_ident_start_char(src[src_i]);
            Lexer_error_occurred = true;
            if (!is_identifier)
                fprintf(stderr, "unknown token '%c'. line %u, column %u.\n",
                        src[src_i], line_num, column_num);
            else {
                unsigned ident_len = get_identifier_len(&src[src_i]);
                char *ident = safe_malloc((ident_len+1)*sizeof(*ident));
                strncpy(ident, &src[src_i], ident_len);
                ident[ident_len] = '\0';
                fprintf(stderr, "unknown token '%s'. line %u, column %u.\n",
                        ident, line_num, column_num);
                m_free(ident);
                column_num += ident_len-1;
                src_i += ident_len-1;
            }
        }

    }

    return Lexer_create(token_tbl);

}
