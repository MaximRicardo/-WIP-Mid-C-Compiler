#pragma once

#include "ast.h"
#include "lexer.h"

struct Expr* Parser_parse(const struct Lexer *lexer);
