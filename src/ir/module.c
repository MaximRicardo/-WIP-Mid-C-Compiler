#include "module.h"
#include "array_lit.h"
#include "func.h"
#include <string.h>

struct IRModule IRModule_init(void) {

    struct IRModule module;
    module.funcs = IRFuncList_init();
    module.array_lits = IRArrayLitList_init();
    return module;

}

struct IRModule IRModule_create(struct IRFuncList funcs,
        struct IRArrayLitList array_lits) {

    struct IRModule module;
    module.funcs = funcs;
    module.array_lits = array_lits;
    return module;

}

void IRModule_free(struct IRModule module) {

    while (module.array_lits.size > 0)
        IRArrayLitList_pop_back(&module.array_lits, IRArrayLit_free);
    IRArrayLitList_free(&module.array_lits);

    while (module.funcs.size > 0)
        IRFuncList_pop_back(&module.funcs, IRFunc_free);
    IRFuncList_free(&module.funcs);

}

u32 IRModule_find_func(const struct IRModule *module, const char *name) {

    u32 i;

    for (i = 0; i < module->funcs.size; i++) {
        if (strcmp(module->funcs.elems[i].name, name) == 0)
            return i;
    }

    return m_u32_max;

}
