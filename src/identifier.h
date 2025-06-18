#pragma once

#include "prim_type.h"
#include "type_mods.h"
#include "typedef.h"
#include "token.h"

/* returns what kind of type specifier ident is. if it isn't one, the function
 * returns PrimType_INVALID. */
enum PrimitiveType Ident_type_spec(const char *ident,
        const struct TypedefList *typedefs);

/* goes through the typedefs and returns the lvls of indir the typedef has.
 * if the identifier is not in typedefs, returns 0. */
u32 Ident_type_lvls_of_indir(const char *ident,
        const struct TypedefList *typedefs);

struct TypeModifiers Ident_type_modifiers(const char *ident,
        const struct TypedefList *typedefs);

/* stuff like const and static. returns TokenType_NONE if it isn't a modifier */
enum TokenType Ident_modifier_str_to_tok(const char *ident);
