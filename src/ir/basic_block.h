#pragma once

/* a basic block consists of several instructions */

#include "instr.h"
#include "../utils/u32_list.h"
#include "../utils/string_list.h"

struct IRFunc;

struct IRBasicBlock {

    /* the label goes at the very beginning of the block */
    char *label;
    struct IRInstrList instrs;
    /* contains block indices in the IRFunc */
    struct U32List imm_doms;

};

struct IRBasicBlock IRBasicBlock_init(void);
struct IRBasicBlock IRBasicBlock_create(char *label,
        struct IRInstrList instrs, struct U32List dom_frontiers);
void IRBasicBlock_free(struct IRBasicBlock block);
/* returns m_u32_max if self has no dominators */
u32 IRBasicBlock_find_common_dom(const struct IRBasicBlock *self,
        const struct IRFunc *parent);
struct ConstStringList IRBasicblock_get_vregs(const struct IRBasicBlock *self,
        bool skip_reserved_regs);

struct IRBasicBlockList {

    struct IRBasicBlock *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(IRBasicBlockList, struct IRBasicBlock)
