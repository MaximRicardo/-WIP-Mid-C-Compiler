#include "arg_to_esp.h"
#include "../../utils/dyn_str.h"
#include "../../utils/math_funcs.h"
#include "../name_mangling.h"
#include <assert.h>
#include <string.h>

static u32 func_arg_idx(const char *vreg, const struct IRFunc *func)
{
    u32 i;

    for (i = 0; i < func->args.size; i++) {

        if (strcmp(func->args.elems[i].name, vreg) == 0)
            return i;
    }

    return m_u32_max;
}

static u32 func_arg_esp_offset(const struct IRFunc *func, const char *arg)
{
    u32 i;

    u32 stack_size = IRFunc_get_stack_size(func);
    /* the +4 accounts for the return address. */
    u32 offset = stack_size + 4;

    for (i = 0; i < func->args.size; i++) {

        const struct IRFuncArg *cur_arg = &func->args.elems[i];

        if (strcmp(cur_arg->name, arg) == 0)
            return offset;

        offset += IRDataType_real_width(&cur_arg->type) / 8;
        offset = u_round_up(offset, 4);
    }

    assert(false);
}

/* returns the idx of the esp offset vreg in func->vregs */
static u32 esp_offset_to_vreg(u32 offset, struct IRFunc *func)
{
    u32 vreg_idx;
    struct DynamicStr esp = DynamicStr_init();

    DynamicStr_append_printf(&esp, "__esp(%u)", offset);

    vreg_idx = StringList_find(&func->vregs, esp.str);
    if (vreg_idx == m_u32_max) {
        StringList_push_back(&func->vregs, esp.str);
        vreg_idx = func->vregs.size - 1;
    } else {
        DynamicStr_free(esp);
    }

    return vreg_idx;
}

static void instrarg_args_to_esp(struct IRInstrArg *arg, struct IRFunc *func)
{
    u32 offset;
    const char *vreg = NULL;
    u32 new_vreg;
    char *demangled = NULL;

    if (arg->type != IRInstrArg_REG)
        return;

    vreg = arg->value.reg_name;
    demangled = IR_demangle(vreg);

    if (func_arg_idx(demangled, func) == m_u32_max)
        goto clean_up_and_ret;

    offset = func_arg_esp_offset(func, demangled);
    new_vreg = esp_offset_to_vreg(offset, func);

    IRFunc_replace_vreg(func, StringList_find(&func->vregs, vreg), new_vreg);

clean_up_and_ret:
    m_free(demangled);
}

static void instr_args_to_esp(struct IRInstr *instr, struct IRFunc *func)
{
    u32 i;

    for (i = 0; i < instr->args.size; i++) {

        instrarg_args_to_esp(&instr->args.elems[i], func);
    }
}

static void block_args_to_esp(struct IRBasicBlock *block, struct IRFunc *func)
{

    u32 i;

    for (i = 0; i < block->instrs.size; i++) {

        instr_args_to_esp(&block->instrs.elems[i], func);
    }
}

static void func_args_to_esp(struct IRFunc *func)
{

    u32 i;

    for (i = 0; i < func->blocks.size; i++) {

        block_args_to_esp(&func->blocks.elems[i], func);
    }
}

void X86_arg_to_esp(struct IRModule *module)
{

    u32 i;

    for (i = 0; i < module->funcs.size; i++) {

        func_args_to_esp(&module->funcs.elems[i]);
    }
}
