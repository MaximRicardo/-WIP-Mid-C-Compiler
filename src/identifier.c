#include "identifier.h"
#include <string.h>

enum PrimitiveType Ident_type_spec(const char *ident,
        const struct TypedefList *typedefs) {

    u32 i;

    if (strcmp(ident, "char") == 0)
        return PrimType_CHAR;
    else if (strcmp(ident, "int") == 0)
        return PrimType_INT;
    else if (strcmp(ident, "void") == 0)
        return PrimType_VOID;

    for (i = 0; i < typedefs->size; i++) {
        if (strcmp(ident, typedefs->elems[i].type_name) == 0) {
            return typedefs->elems[i].conv_type;
        }
    }

    return PrimType_INVALID;

}

enum PrimitiveType Ident_type_lvls_of_indir(const char *ident,
        const struct TypedefList *typedefs) {

    u32 i;

    for (i = 0; i < typedefs->size; i++) {
        if (strcmp(ident, typedefs->elems[i].type_name) == 0)
            return typedefs->elems[i].conv_lvls_of_indir;
    }

    return 0;

}
