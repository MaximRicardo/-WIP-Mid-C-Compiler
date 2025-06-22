#pragma once

#include "lexer.h"
#include "transl_unit.h"

extern bool Parser_error_occurred;

struct TranslUnit Parser_parse(const struct Lexer *lexer);
