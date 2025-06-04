#pragma once

#include "token.h"
#include "bool.h"

struct Lexer {

    bool error;
    struct TokenList token_tbl;

};

struct Lexer Lexer_init(void);

void Lexer_free(struct Lexer *lexer);

/* Converts a string into a list of tokens */
struct Lexer Lexer_lex(const char *src);
