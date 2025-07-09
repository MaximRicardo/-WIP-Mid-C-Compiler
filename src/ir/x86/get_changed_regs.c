#include "get_changed_regs.h"
#include <assert.h>

static void get_block_changed_vregs(const struct IRBasicBlock *block,
                                    struct ConstStringList *chngd_regs)
{
    u32 i;

    for (i = 0; i < block->instrs.size; i++) {
        struct IRInstr *instr = &block->instrs.elems[i];
        const char *dest = NULL;

        if (!IRInstrType_has_dest_reg(instr->type))
            continue;

        assert(instr->args.size > 1);
        assert(instr->args.elems[Arg_SELF].type == IRInstrArg_REG);

        dest = instr->args.elems[Arg_SELF].value.reg_name;
        ConstStringList_push_back(chngd_regs, dest);
    }
}

struct ConstStringList X86_func_get_changed_vregs(const struct IRFunc *func)
{
    u32 i;

    struct ConstStringList regs = ConstStringList_init();

    for (i = 0; i < func->blocks.size; i++) {
        get_block_changed_vregs(&func->blocks.elems[i], &regs);
    }

    return regs;
}
