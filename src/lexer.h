#pragma once

#include "bool.h"
#include "token.h"

struct Lexer {

    struct TokenList token_tbl;

};

extern bool Lexer_error_occurred;

struct Lexer Lexer_init(void);

void Lexer_free(struct Lexer *lexer);

/* Converts a string into a list of tokens */
struct Lexer Lexer_lex(const char *src);
