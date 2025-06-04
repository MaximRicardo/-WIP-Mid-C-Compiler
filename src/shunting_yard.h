#pragma once

#include "ast.h"
#include "bool.h"
#include "token.h"

extern bool SY_error_occurred;

struct Expr* shunting_yard(const struct Token *tokens, u32 lower_bound,
        u32 upper_bound);
