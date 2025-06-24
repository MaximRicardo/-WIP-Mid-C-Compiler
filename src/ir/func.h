#pragma once

/* a function consists of several basic blocks */

#include "basic_block.h"
#include "data_types.h"

struct IRFuncArg {

    struct IRDataType type;
    char *name;

};

struct IRFuncArg IRFuncArg_init(void);
struct IRFuncArg IRFuncArg_create(struct IRDataType type, char *name);
void IRFuncArg_free(struct IRFuncArg arg);

struct IRFuncArgList {

    struct IRFuncArg *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(IRFuncArgList, struct IRFuncArg)

struct IRFunc {

    char *name;
    struct IRDataType ret_type;
    struct IRFuncArgList args;
    struct IRBasicBlockList blocks;

};

struct IRFunc IRFunc_init(void);
struct IRFunc IRFunc_create(char *name, struct IRDataType ret_type,
        struct IRFuncArgList args, struct IRBasicBlockList blocks);
void IRFunc_free(struct IRFunc func);

struct IRFuncList {

    struct IRFunc *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(IRFuncList, struct IRFunc)
