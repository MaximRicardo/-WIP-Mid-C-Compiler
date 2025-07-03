#pragma once

/* a module represents a translation unit. */

#include "func.h"
#include "array_lit.h"

struct IRModule {

    struct IRFuncList funcs;
    struct IRArrayLitList array_lits;

};

struct IRModule IRModule_init(void);
struct IRModule IRModule_create(struct IRFuncList funcs,
        struct IRArrayLitList array_lits);
void IRModule_free(struct IRModule module);
u32 IRModule_find_func(const struct IRModule *module, const char *name);
