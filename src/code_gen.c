/* DEPRACATED! USE THE IR FOR CODE GENERATION! */

#include "code_gen.h"
#include "structs.h"

/* The architecture to use */
#include "x86/code_gen.h"

void CodeGen_generate(FILE *output, const struct BlockNode *ast,
                      const struct StructList *structs)
{
    CodeGenArch_generate(output, ast, structs);
}
