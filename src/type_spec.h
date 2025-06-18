#pragma once

#include "comp_dependent/ints.h"
#include "token.h"
#include "prim_type.h"
#include "typedef.h"

/* returns the index of the next token after the type name and the asterisks.
 * if *error_occurred is already equal to true, it will stay true. */
u32 read_type_spec(const struct TokenList *token_tbl, u32 type_spec_idx,
        enum PrimitiveType *type, unsigned *lvls_of_indir,
        const struct TypedefList *typedefs, bool *error_occurred);
