#include "reg_lifetimes.h"
#include "../../utils/make_str_cpy.h"
#include <stdio.h>
#include <string.h>

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

static void get_block_reg_lts(const struct IRBasicBlock *block, u32 *n_instrs,
        struct IRRegLTList *lts) {

    u32 i;

    for (i = 0; i < block->instrs.size; i++) {

        get_instr_reg_lts(&block->instrs.elems[i], n_instrs, lts);

    }

}

struct IRRegLTList IRRegLTList_get_func_lts(const struct IRFunc *func) {

    struct IRRegLTList lts = IRRegLTList_init();

    u32 i;
    u32 n_instrs = 0;

    for (i = 0; i < func->blocks.size; i++) {

        get_block_reg_lts(&func->blocks.elems[i], &n_instrs, &lts);

    }

    for (i = 0; i < lts.size; i++) {

        printf("reg '%s' created at %u, dies at %u.\n",
                lts.elems[i].reg_name, lts.elems[i].creation_idx,
                lts.elems[i].death_idx);

    }

    return lts;

}
