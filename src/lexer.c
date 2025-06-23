#include "lexer.h"
#include "comp_dependent/ints.h"
#include "err_msg.h"
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

    while (lexer->token_tbl.size > 0) {
        TokenList_pop_back(&lexer->token_tbl, Token_free);
    }
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

/* returns TokenType_NONE if the identifier isn't a keyword */
static enum TokenType identifier_keyword(const char *ident_start,
        u32 ident_len) {

    if (strncmp(ident_start, "if", ident_len) == 0 && ident_len == 2)
        return TokenType_IF_STMT;
    if (strncmp(ident_start, "else", ident_len) == 0 && ident_len == 4)
        return TokenType_ELSE;
    else if (strncmp(ident_start, "while", ident_len) == 0 && ident_len == 5)
        return TokenType_WHILE_STMT;
    else if (strncmp(ident_start, "for", ident_len) == 0 && ident_len == 3)
        return TokenType_FOR_STMT;
    else if (strncmp(ident_start, "typedef", ident_len) == 0 && ident_len == 7)
        return TokenType_TYPEDEF;
    else if (strncmp(ident_start, "static", ident_len) == 0 && ident_len == 6)
        return TokenType_STATIC;
    else if (strncmp(ident_start, "signed", ident_len) == 0 && ident_len == 6)
        return TokenType_SIGNED;
    else if (strncmp(ident_start, "unsigned", ident_len) == 0 &&
            ident_len == 8)
        return TokenType_UNSIGNED;
    else if (strncmp(ident_start, "struct", ident_len) == 0 && ident_len == 6)
        return TokenType_STRUCT;
    else
        return TokenType_NONE;

}

static int escape_code_to_int(char code, unsigned line_num,
        unsigned column_num, const char *file_path) {

    switch (code) {

    case 'a':
        return '\a';

    case 'b':
        return '\b';

    case 'f':
        return '\f';

    case 'n':
        return '\n';

    case 'r':
        return '\r';

    case 't':
        return '\t';

    case 'v':
        return '\v';

    case '\\':
        return '\\';

    case '\'':
        return '\'';

    case '\"':
        return '\"';

    case '\?':
        return '\?';

    case '0':
        return '\0';

    default:
        ErrMsg_print(ErrMsg_on, &Lexer_error_occurred, file_path,
                "invalid escape sequence on line %u, column %u.\n",
                line_num, column_num);
        return 0;

    }

}

/* end_idx points to the closing single quote */
static int read_single_quote_str(const char *src, u32 single_qt_idx,
        u32 *end_idx, unsigned line_num, unsigned column_num,
        const char *file_path) {

    int value;

    assert(src[single_qt_idx] == '\'');

    if (src[single_qt_idx+1] == '\\') {
        value = escape_code_to_int(src[single_qt_idx+2], line_num, column_num,
                file_path);
        *end_idx = single_qt_idx+3;
    }
    else if (src[single_qt_idx+1] == '\n') {
        ErrMsg_print(ErrMsg_on, &Lexer_error_occurred, file_path,
                "missing terminating single quote for the one on line"
                " %u, column %u.\n", line_num, column_num);
        *end_idx = single_qt_idx;
        value = 0;
    }
    else {
        value = src[single_qt_idx+1];
        *end_idx = single_qt_idx+2;
    }

    return value;

}

/* returns the index of the closing double quote */
static int read_string(const char *src, u32 str_start, unsigned line_num,
        unsigned column_num, struct TokenList *token_tbl,
        const char *file_path) {

    union TokenValue value;

    u32 src_i = str_start+1;
    u32 cur_column_num = column_num+1;
    char *string = NULL;
    u32 string_len = 0;
    /* must be initialized to a value greater than 0 */
    u32 string_capacity = 128;

    assert(src[str_start] == '\"');

    string = safe_malloc(string_capacity*sizeof(*string));

    while (src[src_i] != '\0' && src[src_i] != '\"' && src[src_i] != '\n') {

        if (string_len >= string_capacity) {
            string_capacity *= string_capacity;
            string = safe_realloc(string, string_capacity*sizeof(*string));
        }

        if (src[src_i-1] == '\\') {
            string[string_len-1] = escape_code_to_int(src[src_i], line_num,
                    cur_column_num, file_path);
            ++src_i;
        }
        else
            string[string_len++] = src[src_i++];
        ++cur_column_num;

    }

    string = safe_realloc(string, (string_len+1)*sizeof(*string));
    string[string_len++] = '\0';

    value.string = string;
    TokenList_push_back(token_tbl, Token_create_w_val(line_num, column_num,
                &src[str_start+1], src_i-str_start, file_path,
                TokenType_STR_LIT, value));

    if (src[src_i] == '\0' || src[src_i] == '\n') {
        ErrMsg_print(ErrMsg_on, &Lexer_error_occurred, file_path,
                "expected a closing '\"' for the string on line %u,"
                " column %u.\n", line_num, column_num);
        Lexer_error_occurred = true;
        --src_i;
    }

    return src_i;

}

/* returns m_u32_max if the instance couldn't be found */
static u32 find_macro_instance(const struct MacroInstList *macro_insts,
        u32 inst_start_idx) {

    u32 i;

    for (i = 0; i < macro_insts->size; i++) {
        if (macro_insts->elems[i].start_idx == inst_start_idx)
            return i;
    }

    return m_u32_max;

}

static void lex_str(const char *src, const char *file_path,
        const struct MacroInstList *macro_insts, unsigned start_line_num,
        unsigned start_column_num, u32 start_i, struct Lexer *lexer,
        bool inc_line_n_column_num) {

    struct TokenList *token_tbl = &lexer->token_tbl;
    u32 src_len;
    u32 src_i;
    unsigned line_num = start_line_num;
    unsigned column_num = start_column_num;

    if (!src)
        return;

    src_len = strlen(src);

    Lexer_error_occurred = false;

    for (src_i = start_i; src[src_i] != '\0';
            src_i++,column_num++) {

        if (!inc_line_n_column_num) {
            line_num = start_line_num;
            column_num = start_column_num;
        }

        if (valid_ident_start_char(src[src_i]) &&
                find_macro_instance(macro_insts, src_i) != m_u32_max) {

            u32 inst_idx = find_macro_instance(macro_insts, src_i);
            u32 ident_len = get_identifier_len(&src[src_i]);

            lex_str(macro_insts->elems[inst_idx].expansion, file_path,
                    macro_insts, line_num, column_num, 0, lexer, false);

            column_num += ident_len-1;
            src_i += ident_len-1;

        }

        else if (isspace(src[src_i])) {
            if (src[src_i] == '\n') {
                ++line_num;
                column_num = 0;
            }
        }
        else if ((src[src_i] == '/' && src[src_i+1] == '/') ||
                src[src_i] == '#') {
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
                ++column_num;
            }
            ++src_i;
            ++column_num;
        }

        else if (src_i+2 < src_len && src[src_i] == '.' &&
                src[src_i+1] == '.' && src[src_i+2] == '.') {
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 2, file_path, TokenType_VARIADIC));
            src_i += 2;
            column_num += 2;
        }

        else if (src[src_i] == '|' && src[src_i+1] == '|') {
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 2, file_path, TokenType_BOOLEAN_OR));
            ++src_i;
            ++column_num;
        }
        else if (src[src_i] == '&' && src[src_i+1] == '&') {
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 2, file_path, TokenType_BOOLEAN_AND));
            ++src_i;
            ++column_num;
        }
        else if (src[src_i] == '=' && src[src_i+1] == '=') {
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 2, file_path, TokenType_EQUAL_TO));
            ++src_i;
            ++column_num;
        }
        else if (src[src_i] == '!' && src[src_i+1] == '=') {
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 2, file_path, TokenType_NOT_EQUAL_TO));
            ++src_i;
            ++column_num;
        }
        else if (src[src_i] == '<' && src[src_i+1] == '=') {
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 2, file_path, TokenType_L_THAN_OR_E));
            ++src_i;
            ++column_num;
        }
        else if (src[src_i] == '>' && src[src_i+1] == '=') {
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 2, file_path, TokenType_G_THAN_OR_E));
            ++src_i;
            ++column_num;
        }
        else if (src[src_i] == '+' && src[src_i+1] == '+') {
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 2, file_path, TokenType_PREFIX_INC));
            ++src_i;
            ++column_num;
        }
        else if (src[src_i] == '-' && src[src_i+1] == '-') {
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 2, file_path, TokenType_PREFIX_DEC));
            ++src_i;
            ++column_num;
        }
        else if (src[src_i] == '-' && src[src_i+1] == '>') {
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 2, file_path,
                        TokenType_MEMBER_ACCESS_PTR));
            ++src_i;
            ++column_num;
        }

        else if (src[src_i] == ';')
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, file_path, TokenType_SEMICOLON));

        else if (src[src_i] == '+')
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, file_path, TokenType_PLUS));
        else if (src[src_i] == '-')
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, file_path, TokenType_MINUS));
        else if (src[src_i] == '*')
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, file_path, TokenType_MUL));
        else if (src[src_i] == '/')
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, file_path, TokenType_DIV));
        else if (src[src_i] == '%')
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, file_path, TokenType_MODULUS));
        else if (src[src_i] == '=')
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, file_path, TokenType_EQUAL));
        else if (src[src_i] == ',')
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, file_path, TokenType_COMMA));
        else if (src[src_i] == '[')
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, file_path, TokenType_L_ARR_SUBSCR));
        else if (src[src_i] == '&')
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, file_path, TokenType_BITWISE_AND));
        else if (src[src_i] == '<')
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, file_path, TokenType_L_THAN));
        else if (src[src_i] == '>')
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, file_path, TokenType_G_THAN));
        else if (src[src_i] == '.')
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, file_path, TokenType_MEMBER_ACCESS));

        else if (src[src_i] == '~')
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, file_path, TokenType_BITWISE_NOT));
        else if (src[src_i] == '!')
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, file_path, TokenType_BOOLEAN_NOT));

        else if (src[src_i] == '(')
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, file_path, TokenType_L_PAREN));
        else if (src[src_i] == ')')
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, file_path, TokenType_R_PAREN));
        else if (src[src_i] == '{')
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, file_path, TokenType_L_CURLY));
        else if (src[src_i] == '}')
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, file_path, TokenType_R_CURLY));
        else if (src[src_i] == ']')
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, file_path, TokenType_R_ARR_SUBSCR));

        else if (isdigit(src[src_i])) {
            /* An integer literal */
            char *end_ptr = NULL;
            u32 chars_moved;
            union TokenValue value;
            value.int_value = strtoul(&src[src_i], &end_ptr, 0);
            chars_moved = end_ptr-&src[src_i];
            TokenList_push_back(token_tbl, Token_create_w_val(line_num,
                        column_num, &src[src_i], chars_moved, file_path,
                        TokenType_INT_LIT, value));
            src_i += chars_moved-1;
            column_num += chars_moved-1;
        }

        else if (src[src_i] == '\'') {
            u32 end_idx;
            union TokenValue value;
            value.int_value = read_single_quote_str(src, src_i, &end_idx,
                    line_num, column_num, file_path);
            TokenList_push_back(token_tbl, Token_create_w_val(line_num,
                        column_num, &src[src_i], end_idx-src_i+1, file_path,
                        TokenType_INT_LIT, value));
            column_num += end_idx-src_i;
            src_i = end_idx;
        }
        else if (src[src_i] == '\"') {
            u32 end_idx = read_string(src, src_i, line_num, column_num,
                    token_tbl, file_path);
            column_num += end_idx-src_i;
            src_i = end_idx;
        }

        else if (valid_ident_start_char(src[src_i])) {
            unsigned len = get_identifier_len(&src[src_i]);

            enum TokenType keyword_type = identifier_keyword(&src[src_i], len);

            if (keyword_type != TokenType_NONE) {
                TokenList_push_back(token_tbl,
                        Token_create(
                            line_num, column_num, &src[src_i], len, file_path,
                            keyword_type
                            )
                        );
            }
            else {
                TokenList_push_back(token_tbl,
                        Token_create(
                            line_num, column_num, &src[src_i], len, file_path,
                            TokenType_IDENT
                            )
                        );
            }
            src_i += len-1;
            column_num += len-1;
        }

        else if (src[src_i] == ':')
            TokenList_push_back(token_tbl, Token_create(line_num, column_num,
                        &src[src_i], 1, file_path, TokenType_DEBUG_PRINT_RAX));

        else {
            ErrMsg_print(ErrMsg_on, &Lexer_error_occurred, file_path,
                    "unknown token '%c'. line %u, column %u.\n",
                    src[src_i], line_num, column_num);
        }

    }

}

struct Lexer Lexer_lex(const char *src, const char *file_path,
        const struct MacroInstList *macro_insts) {

    struct Lexer lexer = Lexer_init();

    lex_str(src, file_path, macro_insts, 1, 1, 0, &lexer, true);

    return lexer;

}
