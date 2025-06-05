#pragma once

#include "ast.h"
#include "lexer.h"
#include "vector_impl.h"

struct ParserVar {

    char *name;
    enum PrimitiveType type;
    u32 stack_pos;

};

struct ParserVar ParserVar_init(void);
struct ParserVar ParserVar_create(char *name, enum PrimitiveType type,
        u32 stack_pos);
void ParserVar_free(struct ParserVar var);

struct ParVarList {

    struct ParserVar *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(ParVarList, struct ParserVar)

/* returns m_u32_max if there is no var in the current scope by that name */
u32 ParVarList_find_var(const struct ParVarList *self, char *name);

extern bool Parser_error_occurred;

struct TUNode* Parser_parse(const struct Lexer *lexer);
