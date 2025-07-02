#pragma once

/* a module represents a translation unit. */

#include "func.h"

struct IRModule {

    struct IRFuncList funcs;

};

struct IRModule IRModule_init(void);
struct IRModule IRModule_create(struct IRFuncList funcs);
void IRModule_free(struct IRModule module);
u32 IRModule_find_func(const struct IRModule *module, const char *name);
