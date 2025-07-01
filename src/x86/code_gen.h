/* DEPRECATED! COMPILE USING MCCIR INSTEAD! */

#pragma once

#include "../front_end/ast.h"
#include <stdio.h>

void CodeGenArch_generate(FILE *output, const struct BlockNode *ast,
        const struct StructList *structs);
