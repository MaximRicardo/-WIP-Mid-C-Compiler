#include "token.h"
#include "safe_mem.h"
#include "vector_impl.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

struct Token Token_create(unsigned line_num, unsigned column_num,
        enum TokenType type) {

    struct Token token;
    token.line_num = line_num;
    token.column_num = column_num;
    token.type = type;
    memset(&token.value, 0, sizeof(token.value));
    return token;

}

bool Token_is_operator(enum TokenType type) {

    return type == TokenType_PLUS || type == TokenType_MINUS ||
        type == TokenType_MUL || type == TokenType_DIV;

}

struct Token Token_create_w_val(unsigned line_num, unsigned column_num,
        enum TokenType type, union TokenValue value) {

    struct Token token = Token_create(line_num, column_num, type);
    token.value = value;
    return token;

}

unsigned Token_precedence(enum TokenType type) {

    switch (type) {

    case TokenType_PLUS:
    case TokenType_MINUS:
        return 4;

    case TokenType_MUL:
    case TokenType_DIV:
        return 3;

    default:
        assert(false);

    }

}

bool Token_l_to_right_asso(enum TokenType type) {

    unsigned precedence = Token_precedence(type);
    return precedence != 2 && precedence != 13 && precedence != 14;

}

m_define_VectorImpl_init(TokenList)
m_define_VectorImpl_free(TokenList)
m_define_VectorImpl_push_back(TokenList, struct Token)
m_define_VectorImpl_pop_back(TokenList)
