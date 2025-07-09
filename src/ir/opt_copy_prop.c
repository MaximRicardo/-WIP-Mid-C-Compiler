#include "opt_copy_prop.h"
#include "basic_block.h"
#include "func.h"
#include "instr.h"
#include <assert.h>

static bool mov_copy_prop(struct IRInstr *instr, struct IRBasicBlock *cur_block,
                          struct IRFunc *cur_func)
{
    bool used_in_phi;

    assert(instr->args.size == 2);
    assert(instr->args.elems[Arg_SELF].type == IRInstrArg_REG);

    used_in_phi = IRFunc_vreg_in_phi_node(
        cur_func, instr->args.elems[Arg_SELF].value.reg_name);

    if (used_in_phi && instr->args.elems[Arg_LHS].type != IRInstrArg_REG) {
        return false;
    }

    IRFunc_replace_vreg_w_instr_arg(cur_func,
                                    instr->args.elems[Arg_SELF].value.reg_name,
                                    &instr->args.elems[Arg_LHS]);

    IRInstrList_erase(&cur_block->instrs, instr - cur_block->instrs.elems,
                      IRInstr_free);

    return true;
}

/* returns true if instr was erased from block->instrs */
static bool instr_copy_prop(struct IRInstr *instr,
                            struct IRBasicBlock *cur_block,
                            struct IRFunc *cur_func)
{
    if (instr->type == IRInstr_MOV) {
        return mov_copy_prop(instr, cur_block, cur_func);

    } else if (IRInstr_const_fold(instr)) {
        return mov_copy_prop(instr, cur_block, cur_func);
    }

    return false;
}

static void block_copy_prop(struct IRBasicBlock *block, struct IRFunc *parent)
{
    u32 i;

    for (i = 0; i < block->instrs.size; i++) {
        i -= instr_copy_prop(&block->instrs.elems[i], block, parent);
    }
}

static void func_copy_prop(struct IRFunc *func)
{
    u32 i;

    for (i = 0; i < func->blocks.size; i++) {
        block_copy_prop(&func->blocks.elems[i], func);
    }
}

void IROpt_copy_prop(struct IRModule *module)
{
    u32 i;

    for (i = 0; i < module->funcs.size; i++) {
        func_copy_prop(&module->funcs.elems[i]);
    }
}
