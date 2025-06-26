#include "dom_frontier.h"
#include "instr.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* checks if there is a branch instruction in block that goes to dest */
static bool does_block_goto_to_block(const struct IRBasicBlock *block,
        const struct IRBasicBlock *dest) {

    u32 i;

    for (i = 0; i < block->instrs.size; i++) {

        struct IRInstr *instr = &block->instrs.elems[i];
        const char *branch_dest = NULL;

        if (!IRInstrType_is_branch(instr->type))
            continue;

        assert(instr->args.elems[0].type == IRInstrArg_STR ||
                instr->args.elems[2].type == IRInstrArg_STR);

        branch_dest = IRInstrType_is_cond_branch(instr->type) ?
            instr->args.elems[2].value.generic_str :
            instr->args.elems[0].value.generic_str;

        if (strcmp(branch_dest, dest->label) == 0)
            return true;

    }

    return false;

}

static void get_block_dom_frontiers(struct IRBasicBlock *block,
        struct IRFunc *func) {

    u32 i;

    printf("block %lu dom frontiers: ", block - func->blocks.elems);

    for (i = 0; i < func->blocks.size; i++) {

        if (!does_block_goto_to_block(&func->blocks.elems[i], block))
            continue;

        printf("%u ", i);
        U32List_push_back(&block->dom_frontiers, i);

    }

    printf("\n");

}

void IRFunc_get_dom_frontiers(struct IRFunc *func) {

    u32 i;

    for (i = 0; i < func->blocks.size; i++) {
        get_block_dom_frontiers(&func->blocks.elems[i], func);
    }

}
