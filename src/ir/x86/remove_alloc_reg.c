#include "remove_alloc_reg.h"

static void remove_alloc_reg_block(struct IRBasicBlock *block) {

    u32 i;

    for (i = 0; i < block->instrs.size; i++) {
        if (block->instrs.elems[i].type != IRInstr_ALLOC_REG)
            continue;

        IRInstrList_erase(&block->instrs, i, IRInstr_free);
        --i;
    }

}

static void remove_alloc_reg_func(struct IRFunc *func) {

    u32 i;

    for (i = 0; i < func->blocks.size; i++) {
        remove_alloc_reg_block(&func->blocks.elems[i]);
    }

}

void X86_remove_alloc_reg(struct IRModule *module) {

    u32 i;

    for (i = 0; i < module->funcs.size; i++) {
        remove_alloc_reg_func(&module->funcs.elems[i]);
    }

}
