#pragma once

/* a function consists of several basic blocks */

#include "basic_block.h"

struct IRFunc {

    char *name;
    struct IRBasicBlockList blocks;

};

struct IRFunc IRFunc_init(void);
struct IRFunc IRFunc_create(char *name, struct IRBasicBlockList blocks);
void IRFunc_free(struct IRFunc func);

struct IRFuncList {

    struct IRFunc *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(IRFuncList, struct IRFunc)
