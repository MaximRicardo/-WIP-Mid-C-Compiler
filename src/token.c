#include "token.h"
#include "safe_mem.h"
#include "vector_impl.h"
#include <assert.h>
#include <string.h>

struct Token Token_create(unsigned line_num, unsigned column_num,
        const char *src_start, unsigned src_len, const char *file_path,
        enum TokenType type) {

    struct Token token;
    token.line_num = line_num;
    token.column_num = column_num;
    token.src_start = src_start;
    token.src_len = src_len;
    token.file_path = file_path;
    token.type = type;
    memset(&token.value, 0, sizeof(token.value));
    return token;

}

struct Token Token_create_w_val(unsigned line_num, unsigned column_num,
        const char *src_start, unsigned src_len, const char *file_path,
        enum TokenType type, union TokenValue value) {

    struct Token token = Token_create(line_num, column_num, src_start, src_len,
            file_path, type);
    token.value = value;
    return token;

}

void Token_free(struct Token token) {

    if (token.type == TokenType_STR_LIT) {
        m_free(token.value.string);
    }

}

bool Token_is_unary_operator(enum TokenType type) {

    return type > TokenType_UNARY_OPS_START &&
        type < TokenType_UNARY_OPS_END;

}

bool Token_is_bin_operator(enum TokenType type) {

    return type > TokenType_BINARY_OPS_START &&
        type < TokenType_BINARY_OPS_END;

}

bool Token_is_operator(enum TokenType type) {

    return Token_is_unary_operator(type) || Token_is_bin_operator(type);

}

bool Token_is_cmp_operator(enum TokenType type) {

    return type == TokenType_EQUAL_TO || type == TokenType_NOT_EQUAL_TO ||
        type == TokenType_L_THAN || type == TokenType_L_THAN_OR_E ||
        type == TokenType_G_THAN || type == TokenType_G_THAN_OR_E;

}

bool Token_is_literal(enum TokenType type) {

    return type == TokenType_INT_LIT;

}

bool Token_convert_to_unary(enum TokenType type) {

    switch (type) {

    case TokenType_PLUS:
        return TokenType_POSITIVE;

    case TokenType_MINUS:
        return TokenType_NEGATIVE;

    case TokenType_MUL:
        return TokenType_DEREFERENCE;

    case TokenType_BITWISE_AND:
        return TokenType_REFERENCE;

    default:
        assert(false);

    }

}

bool Token_has_unary_version(enum TokenType type) {

    return type == TokenType_PLUS || type == TokenType_MINUS ||
        type == TokenType_MUL || type == TokenType_BITWISE_AND;

}

bool Token_data_type_namespace(enum TokenType type) {

    return type == TokenType_STRUCT;

}

unsigned Token_precedence(enum TokenType type) {

    switch (type) {

    case TokenType_COMMA:
        return 15;

    case TokenType_EQUAL:
        return 14;

    case TokenType_BOOLEAN_OR:
        return 12;

    case TokenType_BOOLEAN_AND:
        return 11;

    case TokenType_BITWISE_AND:
        return 8;

    case TokenType_EQUAL_TO:
    case TokenType_NOT_EQUAL_TO:
        return 7;

    case TokenType_L_THAN:
    case TokenType_L_THAN_OR_E:
    case TokenType_G_THAN:
    case TokenType_G_THAN_OR_E:
        return 6;

    case TokenType_PLUS:
    case TokenType_MINUS:
        return 4;

    case TokenType_MUL:
    case TokenType_DIV:
    case TokenType_MODULUS:
        return 3;

    case TokenType_BITWISE_NOT:
    case TokenType_BOOLEAN_NOT:
    case TokenType_POSITIVE:
    case TokenType_NEGATIVE:
    case TokenType_DEREFERENCE:
    case TokenType_REFERENCE:
    case TokenType_PREFIX_INC:
    case TokenType_PREFIX_DEC:
    case TokenType_TYPECAST:
        return 2;

    case TokenType_L_ARR_SUBSCR:
    case TokenType_MEMBER_ACCESS:
    case TokenType_MEMBER_ACCESS_PTR:
    case TokenType_POSTFIX_INC:
    case TokenType_POSTFIX_DEC:
        return 1;

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
