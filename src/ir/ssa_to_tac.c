#include "ssa_to_tac.h"
#include "basic_block.h"
#include "func.h"
#include "instr.h"
#include "../utils/make_str_cpy.h"
#include <assert.h>

/* returns whether or not it erased instr from cur_block->instrs */
static bool instr_ssa_to_tac(struct IRInstr *instr,
        struct IRBasicBlock *cur_block, struct IRFunc *cur_func) {

    u32 i;
    u32 block_common_dom;

    if (instr->type != IRInstr_PHI)
        return false;

    for (i = 1; i < instr->args.size; i++) {
        assert(instr->args.elems[i].type == IRInstrArg_REG);

        IRFunc_rename_vreg(cur_func, instr->args.elems[i].value.reg_name,
                make_str_copy(instr->args.elems[0].value.reg_name));
    }

    block_common_dom = IRBasicBlock_find_common_dom(cur_block, cur_func);
    IRInstrList_push_back(
            &cur_func->blocks.elems[block_common_dom].instrs,
            IRInstr_create_alloc_reg(
                instr->args.elems[0].value.reg_name,
                instr->args.elems[0].data_type
                )
            );

    IRInstrList_erase(
            &cur_block->instrs, instr-cur_block->instrs.elems, IRInstr_free
            );

    return true;

}

static void block_ssa_to_tac(struct IRBasicBlock *block,
        struct IRFunc *cur_func) {

    u32 i;

    for (i = 0; i < cur_func->blocks.size; i++) {

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
