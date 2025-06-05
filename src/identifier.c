#include "identifier.h"
#include "ast.h"
#include <string.h>

enum PrimitiveType Ident_type_spec(const char *ident) {

    if (strcmp(ident, "int") == 0)
        return PrimType_INT;

    return PrimType_INVALID;

}
