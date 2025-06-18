#include "identifier.h"
#include "prim_type.h"
#include "token.h"
#include "type_mods.h"
#include <string.h>

enum PrimitiveType Ident_type_spec(const char *ident,
        const struct TypedefList *typedefs) {

    u32 i;

    if (strcmp(ident, "char") == 0)
        return PrimType_CHAR;
    if (strcmp(ident, "uchar") == 0)
        return PrimType_UCHAR;

    if (strcmp(ident, "short") == 0)
        return PrimType_SHORT;
    if (strcmp(ident, "ushort") == 0)
        return PrimType_USHORT;

    else if (strcmp(ident, "int") == 0)
        return PrimType_INT;
    else if (strcmp(ident, "uint") == 0)
        return PrimType_UINT;

    else if (strcmp(ident, "long") == 0)
        return PrimType_LONG;
    else if (strcmp(ident, "ulong") == 0)
        return PrimType_ULONG;

    else if (strcmp(ident, "void") == 0)
        return PrimType_VOID;

    for (i = 0; i < typedefs->size; i++) {
        if (strcmp(ident, typedefs->elems[i].type_name) == 0) {
            return typedefs->elems[i].conv_type;
        }
    }

    return PrimType_INVALID;

}

u32 Ident_type_lvls_of_indir(const char *ident,
        const struct TypedefList *typedefs) {

    u32 i;

    for (i = 0; i < typedefs->size; i++) {
        if (strcmp(ident, typedefs->elems[i].type_name) == 0)
            return typedefs->elems[i].conv_lvls_of_indir;
    }

    return 0;

}

struct TypeModifiers Ident_type_modifiers(const char *ident,
        const struct TypedefList *typedefs) {

    u32 i;

    for (i = 0; i < typedefs->size; i++) {
        if (strcmp(ident, typedefs->elems[i].type_name) == 0)
            return typedefs->elems[i].conv_mods;
    }

    return TypeModifiers_init();

}

enum TokenType Ident_modifier_str_to_tok(const char *ident) {

    if (strcmp(ident, "static") == 0)
        return TokenType_STATIC;
    else
        return TokenType_NONE;

}
