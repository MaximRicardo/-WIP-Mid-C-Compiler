#include "ssa_to_tac.h"
#include "basic_block.h"
#include "func.h"
#include "instr.h"
#include <assert.h>

static void replace_phi_args(struct IRInstr *instr, struct IRFunc *cur_func) {

    u32 i;

    const struct IRInstrArg *self_arg = &instr->args.elems[Arg_SELF];
    u32 dest_vreg =
        StringList_find(&cur_func->vregs, self_arg->value.reg_name);

    assert(dest_vreg != m_u32_max);

    for (i = 1; i < instr->args.size; i++) {
        u32 old_vreg;
        const struct IRInstrArg *arg = &instr->args.elems[i];

        assert(arg->type == IRInstrArg_REG);

        old_vreg = StringList_find(&cur_func->vregs, arg->value.reg_name);
        assert(old_vreg != m_u32_max);

        if (old_vreg != dest_vreg) {
            IRFunc_replace_vreg(cur_func, old_vreg, dest_vreg);
            dest_vreg -= dest_vreg > old_vreg;
        }
    }

}

/* returns whether or not it erased instr from cur_block->instrs */
static bool instr_ssa_to_tac(struct IRInstr *instr,
        struct IRBasicBlock *cur_block, struct IRFunc *cur_func) {

    u32 block_common_dom;

    if (instr->type != IRInstr_PHI)
        return false;

    assert(instr->args.size > 0);
    assert(instr->args.elems[Arg_SELF].type == IRInstrArg_REG);

    replace_phi_args(instr, cur_func);

    block_common_dom = IRBasicBlock_find_common_dom(cur_block, cur_func);

    /* GET RID OF THIS IF STATEMENT LATER */

    if (block_common_dom != m_u32_max) {
        IRInstrList_push_back(
                &cur_func->blocks.elems[block_common_dom].instrs,
                IRInstr_create_alloc_reg(
                    instr->args.elems[0].value.reg_name,
                    instr->args.elems[0].data_type
                    )
                );
    }

    IRInstrList_erase(
            &cur_block->instrs, instr-cur_block->instrs.elems, IRInstr_free
            );

    return true;

}

static void block_ssa_to_tac(struct IRBasicBlock *block,
        struct IRFunc *cur_func) {

    u32 i;

    for (i = 0; i < block->instrs.size; i++) {

        if (instr_ssa_to_tac(&block->instrs.elems[i], block, cur_func))
            --i;

    }

}

static void func_ssa_to_tac(struct IRFunc *func) {

    u32 i;

    for (i = 0; i < func->blocks.size; i++) {

        block_ssa_to_tac(&func->blocks.elems[i], func);

    }

}

void IR_ssa_to_tac(struct IRModule *module) {

    u32 i;

    for (i = 0; i < module->funcs.size; i++) {

        func_ssa_to_tac(&module->funcs.elems[i]);

    }

}
