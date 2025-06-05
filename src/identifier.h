#pragma once

#include "ast.h"

/* returns what kind of type specifier ident is. if it isn't one, the function
 * returns PrimType_INVALID. */
enum PrimitiveType Ident_type_spec(const char *ident);
