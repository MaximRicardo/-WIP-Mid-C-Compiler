#pragma once

#include "prim_type.h"
#include "typedef.h"

/* returns what kind of type specifier ident is. if it isn't one, the function
 * returns PrimType_INVALID. */
enum PrimitiveType Ident_type_spec(const char *ident,
        const struct TypedefList *typedefs);

/* goes through the typedefs and returns the lvls of indir the typedef has.
 * if the identifier is not in typedefs, returns 0. */
enum PrimitiveType Ident_type_lvls_of_indir(const char *ident,
        const struct TypedefList *typedefs);
