#include "basic_block.h"
#include "instr.h"

struct IRBasicBlock IRBasicBlock_init(void) {

    struct IRBasicBlock block;
    block.label = NULL;
    block.instrs = IRInstrList_init();
    return block;

}

struct IRBasicBlock IRBasicBlock_create(char *label,
        struct IRInstrList instrs) {

    struct IRBasicBlock block;
    block.label = label;
    block.instrs = instrs;
    return block;

}

void IRBasicBlock_free(struct IRBasicBlock block) {

    m_free(block.label);

    while (block.instrs.size > 0) {
        IRInstrList_pop_back(&block.instrs, IRInstr_free);
    }
    IRInstrList_free(&block.instrs);

}

m_define_VectorImpl_funcs(IRBasicBlockList, struct IRBasicBlock)
