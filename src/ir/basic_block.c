#include "basic_block.h"
#include "instr.h"
#include "../attrib.h"

/* TODO: IMPLEMENT FINDING A COMMON DOMINATOR */

struct IRBasicBlock IRBasicBlock_init(void) {

    struct IRBasicBlock block;
    block.label = NULL;
    block.instrs = IRInstrList_init();
    block.dom_frontiers = U32List_init();
    return block;

}

struct IRBasicBlock IRBasicBlock_create(char *label,
        struct IRInstrList instrs, struct U32List dom_frontiers) {

    struct IRBasicBlock block;
    block.label = label;
    block.instrs = instrs;
    block.dom_frontiers = dom_frontiers;
    return block;

}

void IRBasicBlock_free(struct IRBasicBlock block) {

    m_free(block.label);

    while (block.instrs.size > 0) {
        IRInstrList_pop_back(&block.instrs, IRInstr_free);
    }
    IRInstrList_free(&block.instrs);

    while (block.dom_frontiers.size > 0) {
        U32List_pop_back(&block.dom_frontiers, NULL);
    }
    U32List_free(&block.dom_frontiers);

}

u32 IRBasicBlock_find_common_dom(const struct IRBasicBlock *self,
        ATTRIBUTE((unused)) const struct IRFunc *parent) {

    /* i'll implement this later, for now just return the start node */
    if (self->dom_frontiers.size == 0)
        return m_u32_max;
    else
        return 0;

}

m_define_VectorImpl_funcs(IRBasicBlockList, struct IRBasicBlock)
