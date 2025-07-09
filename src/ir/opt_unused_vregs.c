#include "opt_unused_vregs.h"
#include "instr.h"
#include "reg_lifetimes.h"
#include <assert.h>

/* returns true if instr was erased from cur_block->instrs */
static bool instr_opt_unused_vregs(struct IRInstr *instr,
                                   const struct IRRegLTList *vreg_lts,
                                   struct IRBasicBlock *cur_block)
{
    u32 reg_lt_idx;
    const char *vreg_name = NULL;

    if (!IRInstrType_has_dest_reg(instr->type))
        return false;

    assert(instr->args.elems[Arg_SELF].type == IRInstrArg_REG);

    vreg_name = instr->args.elems[Arg_SELF].value.reg_name;

    reg_lt_idx = IRRegLTList_find_reg(vreg_lts, vreg_name);
    assert(reg_lt_idx != m_u32_max);

    if (vreg_lts->elems[reg_lt_idx].creation_idx <
        vreg_lts->elems[reg_lt_idx].death_idx)
        return false;

    assert(vreg_lts->elems[reg_lt_idx].creation_idx ==
           vreg_lts->elems[reg_lt_idx].death_idx);

    /* if the creation_idx and death_idx of the register are the same, that
     * means the register is only assigned, and then never used again, ie. it's
     * an unused register. */
    IRInstrList_erase(&cur_block->instrs, instr - cur_block->instrs.elems,
                      IRInstr_free);

    return true;
}

static void block_opt_unused_vregs(struct IRBasicBlock *block,
                                   const struct IRRegLTList *vreg_lts)
{
    u32 i;

    for (i = 0; i < block->instrs.size; i++) {
        i -= instr_opt_unused_vregs(&block->instrs.elems[i], vreg_lts, block);
    }
}

static void func_opt_unused_vregs(struct IRFunc *func)
{
    u32 i;

    struct IRRegLTList vreg_lts = IRRegLTList_get_func_lts(func);

    for (i = 0; i < func->blocks.size; i++) {
        block_opt_unused_vregs(&func->blocks.elems[i], &vreg_lts);
    }

    IRRegLTList_free(&vreg_lts);
}

void IROpt_unused_vregs(struct IRModule *module)
{
    u32 i;

    for (i = 0; i < module->funcs.size; i++) {
        func_opt_unused_vregs(&module->funcs.elems[i]);
    }
}
