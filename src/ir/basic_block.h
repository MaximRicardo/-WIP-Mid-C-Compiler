#pragma once

/* a basic block consists of several instructions */

#include "instr.h"
#include "../utils/u32_list.h"

struct IRFunc;

struct IRBasicBlock {

    /* the label goes at the very beginning of the block */
    char *label;
    struct IRInstrList instrs;
    /* contains block indices in the IRFunc */
    struct U32List dom_frontiers;

};

struct IRBasicBlock IRBasicBlock_init(void);
struct IRBasicBlock IRBasicBlock_create(char *label,
        struct IRInstrList instrs, struct U32List dom_frontiers);
void IRBasicBlock_free(struct IRBasicBlock block);

struct IRBasicBlockList {

    struct IRBasicBlock *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(IRBasicBlockList, struct IRBasicBlock)
