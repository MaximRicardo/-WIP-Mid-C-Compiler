/* DEPRACATED! USE THE IR FOR CODE GENERATION! */

#pragma once

#include "front_end/ast.h"
#include <stdio.h>

void CodeGen_generate(FILE *output, const struct BlockNode *ast,
        const struct StructList *structs);
