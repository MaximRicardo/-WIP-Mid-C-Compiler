#include "code_gen.h"

/* The architecture to use */
#include "x64/code_gen.h"

void CodeGen_generate(FILE *output, const struct BlockNode *ast) {

    CodeGenArch_generate(output, ast);

}
