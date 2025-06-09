#include "identifier.h"
#include "ast.h"
#include <string.h>

enum PrimitiveType Ident_type_spec(const char *ident) {

    if (strcmp(ident, "char") == 0)
        return PrimType_CHAR;
    else if (strcmp(ident, "int") == 0)
        return PrimType_INT;
    else if (strcmp(ident, "void") == 0)
        return PrimType_VOID;

    return PrimType_INVALID;

}
