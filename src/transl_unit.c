#include "transl_unit.h"
#include "front_end/ast.h"
#include "structs.h"
#include <stddef.h>

struct TranslUnit TranslUnit_init(void)
{
    struct TranslUnit tu;
    tu.ast = NULL;
    tu.structs = NULL;
    return tu;
}

struct TranslUnit TranslUnit_create(struct BlockNode *ast,
                                    struct StructList *structs)
{
    struct TranslUnit tu;
    tu.ast = ast;
    tu.structs = structs;
    return tu;
}

void TranslUnit_free(struct TranslUnit tu)
{
    BlockNode_free_w_self(tu.ast);

    while (tu.structs->size > 0)
        StructList_pop_back(tu.structs, Struct_free);
    StructList_free(tu.structs);
}
