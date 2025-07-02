#include "module.h"
#include "func.h"
#include <string.h>

struct IRModule IRModule_init(void) {

    struct IRModule module;
    module.funcs = IRFuncList_init();
    return module;

}

struct IRModule IRModule_create(struct IRFuncList funcs) {

    struct IRModule module;
    module.funcs = funcs;
    return module;

}

void IRModule_free(struct IRModule module) {

    while (module.funcs.size > 0) {
        IRFuncList_pop_back(&module.funcs, IRFunc_free);
    }
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
