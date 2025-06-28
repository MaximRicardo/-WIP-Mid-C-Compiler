#include "block_dom.h"
#include "instr.h"
#include <string.h>
#include <assert.h>

/* checks if there is a branch instruction in block that goes to dest */
static bool does_block_goto_to_block(const struct IRBasicBlock *block,
        const struct IRBasicBlock *dest) {

    u32 i;

    for (i = 0; i < block->instrs.size; i++) {

        struct IRInstr *instr = &block->instrs.elems[i];

        if (!IRInstrType_is_branch(instr->type))
            continue;

        assert(instr->args.elems[0].type == IRInstrArg_STR ||
                (instr->args.elems[2].type == IRInstrArg_STR &&
                 instr->args.elems[3].type == IRInstrArg_STR));

        if (IRInstrType_is_cond_branch(instr->type)) {
            if (strcmp(instr->args.elems[2].value.generic_str, dest->label)
                    == 0 ||
                strcmp(instr->args.elems[3].value.generic_str, dest->label)
                    == 0) {

                return true;

            }
        }
        else {
            if (strcmp(instr->args.elems[0].value.generic_str, dest->label)
                    == 0)
                return true;
        }

    }

    return false;

}

static void get_block_imm_doms(struct IRBasicBlock *block,
        struct IRFunc *func) {

    u32 i;

    for (i = 0; i < func->blocks.size; i++) {

        if (!does_block_goto_to_block(&func->blocks.elems[i], block))
            continue;

        U32List_push_back(&block->imm_doms, i);

    }

}

void IRFunc_get_blocks_imm_doms(struct IRFunc *func) {

    u32 i;

    for (i = 0; i < func->blocks.size; i++) {
        get_block_imm_doms(&func->blocks.elems[i], func);
    }

}
