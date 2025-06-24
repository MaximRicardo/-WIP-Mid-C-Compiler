#include "func.h"
#include "basic_block.h"
#include "data_types.h"

struct IRFuncArg IRFuncArg_init(void) {

    struct IRFuncArg arg;
    arg.type = IRDataType_init();
    arg.name = NULL;
    return arg;

}

struct IRFuncArg IRFuncArg_create(struct IRDataType type, char *name) {

    struct IRFuncArg arg;
    arg.type = type;
    arg.name = name;
    return arg;

}

void IRFuncArg_free(struct IRFuncArg arg) {

    m_free(arg.name);

}

struct IRFunc IRFunc_init(void) {

    struct IRFunc func;
    func.name = NULL;
    func.ret_type = IRDataType_init();
    func.args = IRFuncArgList_init();
    func.blocks = IRBasicBlockList_init();
    return func;

}

struct IRFunc IRFunc_create(char *name, struct IRDataType ret_type,
        struct IRFuncArgList args, struct IRBasicBlockList blocks) {

    struct IRFunc func;
    func.name = name;
    func.ret_type = ret_type;
    func.args = args;
    func.blocks = blocks;
    return func;

}

void IRFunc_free(struct IRFunc func) {

    m_free(func.name);

    while (func.blocks.size > 0) {
        IRBasicBlockList_pop_back(&func.blocks, IRBasicBlock_free);
    }
    IRBasicBlockList_free(&func.blocks);

    while (func.args.size > 0) {
        IRFuncArgList_pop_back(&func.args, IRFuncArg_free);
    }
    IRFuncArgList_free(&func.args);

}

m_define_VectorImpl_funcs(IRFuncArgList, struct IRFuncArg)
m_define_VectorImpl_funcs(IRFuncList, struct IRFunc)
