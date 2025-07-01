#pragma once

#include "../utils/bool.h"
#include "token.h"
#include "../pre_proc/pre_proc.h"

struct Lexer {

    struct TokenList token_tbl;

};

extern bool Lexer_error_occurred;

struct Lexer Lexer_init(void);

void Lexer_free(struct Lexer *lexer);

/* Converts a string into a list of tokens */
struct Lexer Lexer_lex(const char *src, const char *file_path,
        const struct MacroInstList *macro_insts);
