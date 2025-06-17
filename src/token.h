#pragma once

#include "bool.h"
#include "comp_dependent/ints.h"
#include "vector_impl.h"

/* when adding a new token make sure to update:
 *  Token_precedence, ExprType
 */

enum TokenType {

    TokenType_NONE,

    TokenType_SEMICOLON,

    /* binary operators */
    TokenType_BINARY_OPS_START,
    TokenType_PLUS,
    TokenType_MINUS,
    TokenType_MUL,
    TokenType_DIV,
    TokenType_MODULUS,
    TokenType_EQUAL,
    TokenType_COMMA,
    TokenType_L_ARR_SUBSCR,
    TokenType_BITWISE_AND,
    TokenType_BOOLEAN_OR,
    TokenType_BOOLEAN_AND,
    TokenType_EQUAL_TO,
    TokenType_NOT_EQUAL_TO,
    TokenType_L_THAN,
    TokenType_L_THAN_OR_E,
    TokenType_G_THAN,
    TokenType_G_THAN_OR_E,
    TokenType_BINARY_OPS_END,

    /* unary operators */
    TokenType_UNARY_OPS_START,
    TokenType_BITWISE_NOT,
    TokenType_BOOLEAN_NOT,
    TokenType_REFERENCE,
    TokenType_DEREFERENCE,
    TokenType_POSITIVE,
    TokenType_NEGATIVE,
    TokenType_UNARY_OPS_END,

    TokenType_L_PAREN,
    TokenType_R_PAREN,
    TokenType_L_CURLY,
    TokenType_R_CURLY,
    TokenType_R_ARR_SUBSCR,

    TokenType_INT_LIT,
    TokenType_STR_LIT,

    TokenType_IDENT,
    TokenType_FUNC_CALL,

    /* keywords
     * type specifiers aren't included here cuz they're detected during parsing
     * instead of during lexing */
    TokenType_IF_STMT,
    TokenType_ELSE,
    TokenType_WHILE_STMT,
    TokenType_FOR_STMT,

    TokenType_DEBUG_PRINT_RAX

};

union TokenValue {
    u32 int_value;
    char *string;
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
void Token_free(struct Token token);

bool Token_is_unary_operator(enum TokenType type);
bool Token_is_bin_operator(enum TokenType type);
bool Token_is_operator(enum TokenType type);
bool Token_is_cmp_operator(enum TokenType type);
bool Token_is_literal(enum TokenType type);
/* only works on token types that have a unary equivalent, such as
 * TokenType_MINUS->TokenType_NEGATIVE */
bool Token_convert_to_unary(enum TokenType type);
bool Token_has_unary_version(enum TokenType type);

unsigned Token_precedence(enum TokenType type);
/* Does the token have left to right associativity? */
bool Token_l_to_right_asso(enum TokenType type);

/* uses src_start and src_len to return a null terminated string.
 * the string is dynamically allocated and must be freed */
char* Token_src(const struct Token *self);

m_declare_VectorImpl_funcs(TokenList, struct Token)
