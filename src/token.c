#include "token.h"
#include "safe_mem.h"
#include "vector_impl.h"
#include <assert.h>
#include <string.h>

struct Token Token_create(unsigned line_num, unsigned column_num,
        const char *src_start, unsigned src_len, enum TokenType type) {

    struct Token token;
    token.line_num = line_num;
    token.column_num = column_num;
    token.src_start = src_start;
    token.src_len = src_len;
    token.type = type;
    memset(&token.value, 0, sizeof(token.value));
    return token;

}

struct Token Token_create_w_val(unsigned line_num, unsigned column_num,
        const char *src_start, unsigned src_len, enum TokenType type,
        union TokenValue value) {

    struct Token token = Token_create(line_num, column_num, src_start, src_len,
            type);
    token.value = value;
    return token;

}

bool Token_is_operator(enum TokenType type) {

    return type == TokenType_PLUS || type == TokenType_MINUS ||
        type == TokenType_MUL || type == TokenType_DIV ||
        type == TokenType_MODULUS || type == TokenType_EQUAL;

}

unsigned Token_precedence(enum TokenType type) {

    switch (type) {

    case TokenType_EQUAL:
        return 14;

    case TokenType_PLUS:
    case TokenType_MINUS:
        return 4;

    case TokenType_MUL:
    case TokenType_DIV:
    case TokenType_MODULUS:
        return 3;

    default:
        assert(false);

    }

}

bool Token_l_to_right_asso(enum TokenType type) {

    unsigned precedence = Token_precedence(type);
    return precedence != 2 && precedence != 13 && precedence != 14;

}

char* Token_src(const struct Token *self) {

    char *str = safe_malloc((self->src_len+1)*sizeof(*str));
    strncpy(str, self->src_start, self->src_len);
    str[self->src_len] = '\0';
    return str;

}

m_define_VectorImpl_funcs(TokenList, struct Token)
