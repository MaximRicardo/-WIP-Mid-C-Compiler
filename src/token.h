#pragma once

#include "bool.h"
#include "comp_dependent/ints.h"
#include "vector_impl.h"

/* when adding a new token make sure to update:
 *  Token_is_unary_operator, Token_is_bin_operator, Token_precedence, ExprType
 */

enum TokenType {

    TokenType_NONE,

    TokenType_SEMICOLON,

    /* binary operators */
    TokenType_PLUS,
    TokenType_MINUS,
    TokenType_MUL,
    TokenType_DIV,
    TokenType_MODULUS,
    TokenType_EQUAL,
    TokenType_COMMA,

    /* unary operators */
    TokenType_BITWISE_NOT,

    TokenType_L_PAREN,
    TokenType_R_PAREN,
    TokenType_L_CURLY,
    TokenType_R_CURLY,

    TokenType_INT_LIT,

    TokenType_IDENT,
    TokenType_FUNC_CALL,

    /* keywords
     * type specifiers aren't included here cuz they're detected during parsing
     * instead of during lexing */
    TokenType_IF_STMT,
    TokenType_ELSE,

    TokenType_DEBUG_PRINT_RAX

};

union TokenValue {
    u32 int_value;
};

struct Token {

    unsigned line_num;
    unsigned column_num;

    /* Start of the part of the source file that represents this token */
    const char *src_start;
    unsigned src_len;

    enum TokenType type;
    union TokenValue value;

};

struct TokenList {

    struct Token *elems;
    u32 size;
    u32 capacity;

};

/* Memsets value to all 0s */
struct Token Token_create(unsigned line_num, unsigned column_num,
        const char *src_start, unsigned src_len, enum TokenType type);
struct Token Token_create_w_val(unsigned line_num, unsigned column_num,
        const char *src_start, unsigned src_len, enum TokenType type,
        union TokenValue value);

bool Token_is_unary_operator(enum TokenType type);
bool Token_is_bin_operator(enum TokenType type);
bool Token_is_operator(enum TokenType type);

m_declare_VectorImpl_funcs(TokenList, struct Token)

unsigned Token_precedence(enum TokenType type);
/* Does the token have left to right associativity? */
bool Token_l_to_right_asso(enum TokenType type);

/* uses src_start and src_len to return a null terminated string.
 * the string is dynamically allocated and must be freed */
char* Token_src(const struct Token *self);
