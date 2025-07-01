#pragma once

#include "token.h"

/* converts every binary operator that should be converted to it's unary
 * equivalent, into it's unary equivalent */
void BinToUnary_convert(struct TokenList *token_tbl);
