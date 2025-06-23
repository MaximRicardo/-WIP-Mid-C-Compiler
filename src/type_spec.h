#pragma once

#include "comp_dependent/ints.h"
#include "structs.h"
#include "token.h"
#include "prim_type.h"
#include "typedef.h"
#include "type_mods.h"

/* returns the index of the next token after the type name and the asterisks.
 * if *error_occurred is already equal to true, it will stay true.
 * type_spec_idx can also point to a type modifier like const or static. */
u32 TypeSpec_read(const struct TokenList *token_tbl, u32 type_spec_idx,
        enum PrimitiveType *type, u32 *type_idx, unsigned *lvls_of_indir,
        struct TypeModifiers *mods,
        const struct TypedefList *typedefs, const struct StructList *structs,
        bool *error_occurred);
