#pragma once

#include "ast.h"
#include "lexer.h"

struct Expr* shunting_yard(const struct Lexer *lexer);
