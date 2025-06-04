#pragma once

#include "bool.h"
#include "ints.h"

enum TokenType {

    TokenType_NONE,

    /* Binary operators */
    TokenType_PLUS,
    TokenType_MINUS,
    TokenType_MUL,
    TokenType_DIV,

    TokenType_INT_LIT

};

union TokenValue {
    u32 int_value;
};

struct Token {

    unsigned line_num;
    unsigned column_num;
    enum TokenType type;
    union TokenValue value;

};

struct TokenList {

    struct Token *elems;
    u32 size;
    u32 capacity;

};

struct Token Token_create(unsigned line_num, unsigned column_num,
        enum TokenType type); /* Memsets value to all 0s */
struct Token Token_create_w_val(unsigned line_num, unsigned column_num,
        enum TokenType type, union TokenValue value);

bool Token_is_operator(enum TokenType type);

struct TokenList TokenList_init(void);
void TokenList_push_back(struct TokenList *self, struct Token value);
void TokenList_free(struct TokenList *self);

unsigned Token_precedence(enum TokenType type);
/* Does the token have left to right associativity? */
bool Token_l_to_right_asso(enum TokenType type);
