#pragma once

#include "ast.h"
#include "lexer.h"

extern bool Parser_error_occurred;

struct BlockNode* Parser_parse(const struct Lexer *lexer);
