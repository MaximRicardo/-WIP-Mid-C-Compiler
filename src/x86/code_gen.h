#pragma once

#include "../ast.h"
#include <stdio.h>

void CodeGenArch_generate(FILE *output, const struct BlockNode *ast);
