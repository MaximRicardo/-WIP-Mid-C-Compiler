#include "reg_lifetimes.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

struct IRRegLT IRRegLT_init(void) {

    struct IRRegLT lifetime;
    lifetime.reg_name = NULL;
    lifetime.creation_idx = 0;
    lifetime.death_idx = 0;
    return lifetime;

}

struct IRRegLT IRRegLT_create(const char *reg_name, u32 creation_idx,
        u32 death_idx) {

    struct IRRegLT lifetime;
    lifetime.reg_name = reg_name;
    lifetime.creation_idx = creation_idx;
    lifetime.death_idx = death_idx;
    return lifetime;

}

u32 IRRegLTList_find_reg(const struct IRRegLTList *self,
        const char *reg_name) {

    u32 i;

    for (i = 0; i < self->size; i++) {
        if (strcmp(reg_name, self->elems[i].reg_name) == 0)
            return i;
    }

    return m_u32_max;

}

m_define_VectorImpl_funcs(IRRegLTList, struct IRRegLT)

static void get_instr_reg_lts(const struct IRInstr *instr, u32 *n_instrs,
        struct IRRegLTList *lts) {

    u32 i;

    for (i = 0; i < instr->args.size; i++) {

        u32 reg_idx;
        const char *reg_name;

        if (instr->args.elems[i].type != IRInstrArg_REG)
            continue;

        reg_name = instr->args.elems[i].value.reg_name;

        /* ignore any vregs whose name starts with '__' */
        if (strncmp(reg_name, "__", 2) == 0)
            continue;

        reg_idx = IRRegLTList_find_reg(lts, reg_name);

        if (reg_idx != m_u32_max) {
            lts->elems[reg_idx].death_idx = *n_instrs;
        }
        else {
            IRRegLTList_push_back(lts, IRRegLT_create(
                        reg_name, *n_instrs, *n_instrs
                        ));
        }

    }

    ++*n_instrs;

}

/* ignore_list contains all the block indices to ignore */
static bool block_exits_use_vreg(const struct IRBasicBlock *block,
        const struct IRFunc *parent, struct U32List *ignore_list,
        const char *vreg) {

    u32 i;
    struct U32List exits;
    bool alloced = false;
    bool uses = false;

    if (!ignore_list) {
        alloced = true;
        ignore_list = safe_malloc(sizeof(*ignore_list));
        *ignore_list = U32List_init();
    }

    U32List_push_back(ignore_list, block - parent->blocks.elems);

    exits = IRBasicBlock_get_exits(block, parent);

    for (i = 0; i < exits.size; i++) {

        const struct IRBasicBlock *exit_b =
            &parent->blocks.elems[exits.elems[i]];

        if (U32List_find(ignore_list, exits.elems[i]) != m_u32_max)
            continue;

        if (IRBasicBlock_uses_vreg(exit_b, vreg, true) ||
                block_exits_use_vreg(exit_b, parent, ignore_list, vreg)) {
            uses = true;
            break;
        }

    }

    if (alloced) {
        U32List_free(ignore_list);
        m_free(ignore_list);
    }
    U32List_free(&exits);
    return uses;

}

/* if the vreg in lt is used in a block exit then it's lifetime is extended to
 * the end of the current block */
static void extend_lifetime_via_exits(const struct IRBasicBlock *block,
        const struct IRFunc *cur_func, struct IRRegLT *lt,
        u32 block_end_instr_idx) {

    bool used = block_exits_use_vreg(block, cur_func, NULL, lt->reg_name);

    if (!used)
        return;

    /* in case this assert ever fires: this might be unnecessary and u should
     * try removing it and see if that breaks anything */
    assert(lt->death_idx <= block_end_instr_idx);
    lt->death_idx = block_end_instr_idx;

}

static void get_block_reg_lts(const struct IRBasicBlock *block,
        const struct IRFunc *cur_func, u32 *n_instrs,
        struct IRRegLTList *lts) {

    u32 i;

    for (i = 0; i < block->instrs.size; i++) {

        get_instr_reg_lts(&block->instrs.elems[i], n_instrs, lts);

    }

    for (i = 0; i < lts->size; i++) {

        extend_lifetime_via_exits(
                block, cur_func, &lts->elems[i], *n_instrs-1
                );

    }

}

struct IRRegLTList IRRegLTList_get_func_lts(const struct IRFunc *func) {

    struct IRRegLTList lts = IRRegLTList_init();

    u32 i;
    u32 n_instrs = 0;

    for (i = 0; i < func->blocks.size; i++) {

        get_block_reg_lts(&func->blocks.elems[i], func, &n_instrs, &lts);

    }

    return lts;

}
