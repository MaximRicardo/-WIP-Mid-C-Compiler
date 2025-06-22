#pragma once

#include "ast.h"
#include <stdio.h>

void CodeGen_generate(FILE *output, const struct BlockNode *ast,
        const struct StructList *structs);
