#include "module.h"
#include "func.h"

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
