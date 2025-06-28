#include "basic_block.h"
#include "instr.h"
#include "func.h"
#include "../attrib.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct IRBasicBlock IRBasicBlock_init(void) {

    struct IRBasicBlock block;
    block.label = NULL;
    block.instrs = IRInstrList_init();
    block.imm_doms = U32List_init();
    return block;

}

struct IRBasicBlock IRBasicBlock_create(char *label,
        struct IRInstrList instrs, struct U32List dom_frontiers) {

    struct IRBasicBlock block;
    block.label = label;
    block.instrs = instrs;
    block.imm_doms = dom_frontiers;
    return block;

}

void IRBasicBlock_free(struct IRBasicBlock block) {

    m_free(block.label);

    while (block.instrs.size > 0) {
        IRInstrList_pop_back(&block.instrs, IRInstr_free);
    }
    IRInstrList_free(&block.instrs);

    while (block.imm_doms.size > 0) {
        U32List_pop_back(&block.imm_doms, NULL);
    }
    U32List_free(&block.imm_doms);

}

ATTRIBUTE((unused))
static struct U32List find_first_doms(const struct IRBasicBlock *block,
        const struct IRFunc *parent) {

    const struct IRBasicBlock *cur_block = block;
    struct U32List first_doms = U32List_init();

    while (cur_block->imm_doms.size > 0) {

        U32List_push_back(&first_doms, cur_block->imm_doms.elems[0]);

        cur_block = &parent->blocks.elems[cur_block->imm_doms.elems[0]];

    }

    return first_doms;

}

/* if there are multiple biggest elems with the same size, this returns the idx
 * of the first one */
ATTRIBUTE((unused))
static u32 biggest_elem_in_u32_arr_idx(const u32 *array, u32 size) {

    u32 i;

    u32 max_value = 0;
    u32 max_value_i = 0;

    for (i = 0; i < size; i++) {

        if (array[i] >= max_value) {
            max_value = array[i];
            max_value_i = i;
        }

    }

    return max_value_i;

}

u32 IRBasicBlock_find_common_dom(const struct IRBasicBlock *self,
        ATTRIBUTE((unused)) const struct IRFunc *parent) {

    if (self->imm_doms.size == 0)
        return m_u32_max;
    return 0;

#ifdef _COMMENT
    u32 i;
    u32 common_dom = m_u32_max;

    /* list of every 0th domintor */
    struct U32List first_doms = find_first_doms(self, parent);
    u32 *n_occurences = NULL;

    struct U32List block_stack;

    if (first_doms.size == 0)
        return m_u32_max;

    n_occurences = safe_calloc(first_doms.size, sizeof(*n_occurences));

    block_stack = U32List_init();
    for (i = 0; i < self->imm_doms.size; i++) {
        U32List_push_back(&block_stack, self->imm_doms.elems[i]);
    }

    while (block_stack.size > 0) {

        const struct IRBasicBlock *cur_block =
            &parent->blocks.elems[U32List_back(&block_stack)];

        U32List_pop_back(&block_stack, NULL);

        for (i = 0; i < first_doms.size; i++) {
            if (first_doms.elems[i] == cur_block-parent->blocks.elems)
                ++n_occurences[i];
        }

        for (i = 0; i < cur_block->imm_doms.size; i++) {
            U32List_push_back(&block_stack, cur_block->imm_doms.elems[i]);
        }

    }

    /* find whichever element in isnt_common_dom which didn't get set to
     * true */
    common_dom = first_doms.elems[
        biggest_elem_in_u32_arr_idx(n_occurences, first_doms.size)
        ];

    U32List_free(&first_doms);
    m_free(n_occurences);
    U32List_free(&block_stack);

    return common_dom;
#endif

}

struct ConstStringList IRBasicblock_get_vregs(
        const struct IRBasicBlock *self, bool skip_reserved_regs) {

    u32 i;
    struct ConstStringList vregs = ConstStringList_init();

    for (i = 0; i < self->instrs.size; i++) {

        u32 j;
        struct IRInstr *instr = &self->instrs.elems[i];

        for (j = 0; j < instr->args.size; j++) {
            struct IRInstrArg *arg = &instr->args.elems[j];

            if (arg->type != IRInstrArg_REG)
                continue;

            if (strncmp(arg->value.reg_name, "__", 2) == 0 &&
                    skip_reserved_regs)
                continue;

            if (ConstStringList_find(&vregs, arg->value.reg_name) == m_u32_max)
                ConstStringList_push_back(&vregs, arg->value.reg_name);
        }

    }

    return vregs;

}

m_define_VectorImpl_funcs(IRBasicBlockList, struct IRBasicBlock)
