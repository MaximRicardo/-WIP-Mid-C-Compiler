#pragma once

/* a basic block consists of several instructions */

#include "instr.h"

struct IRBasicBlock {

    struct IRInstrList instrs;

};

struct IRBasicBlock IRBasicBlock_init(void);
struct IRBasicBlock IRBasicBlock_create(struct IRInstrList instrs);
void IRBasicBlock_free(struct IRBasicBlock block);

struct IRBasicBlockList {

    struct IRBasicBlock *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(IRBasicBlockList, struct IRBasicBlock)
