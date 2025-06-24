#include "func.h"
#include "basic_block.h"

struct IRFunc IRFunc_init(void) {

    struct IRFunc func;
    func.name = NULL;
    func.blocks = IRBasicBlockList_init();
    return func;

}

struct IRFunc IRFunc_create(char *name, struct IRBasicBlockList blocks) {

    struct IRFunc func;
    func.name = name;
    func.blocks = blocks;
    return func;

}

void IRFunc_free(struct IRFunc func) {

    m_free(func.name);

    while (func.blocks.size > 0) {
        IRBasicBlockList_pop_back(&func.blocks, IRBasicBlock_free);
    }
    IRBasicBlockList_free(&func.blocks);

}

m_define_VectorImpl_funcs(IRFuncList, struct IRFunc)
