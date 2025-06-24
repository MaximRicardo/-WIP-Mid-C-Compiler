#include "ir_to_str.h"
#include "../utils/dyn_str.h"
#include "data_types.h"
#include "instr.h"
#include "ir_instr_to_str.h"
#include "data_type_to_str.h"
#include <assert.h>

#define m_tab "    "

static void instr_arg_to_str(struct DynamicStr *output,
        const struct IRInstrArg *arg) {

    char *data_type_str = IR_data_type_to_str(&arg->data_type);

    DynamicStr_append_printf(output, "%s ", data_type_str);

    if (arg->type == IRInstrArg_REG) {
        DynamicStr_append_printf(output, "%%%s", arg->value.reg_name);
    }
    else if (arg->type == IRInstrArg_IMM32 && arg->data_type.is_signed) {
        DynamicStr_append_printf(output, "%d", arg->value.imm_i32);
    }
    else if (arg->type == IRInstrArg_IMM32 && !arg->data_type.is_signed) {
        DynamicStr_append_printf(output, "%u", arg->value.imm_u32);
    }
    else
        assert(false);

    m_free(data_type_str);

}

static void instr_to_str(struct DynamicStr *output,
        const struct IRInstr *instr) {

    u32 i;

    if (instr->type == IRInstr_COMMENT) {
        assert(false);
    }
    else if (instr->type == IRInstr_INVALID) {
        assert(false);
    }

    DynamicStr_append_printf(output, m_tab "%s",
            IR_instr_t_to_str(instr->type));

    for (i = 0; i < instr->args.size; i++) {
        DynamicStr_append(output, " ");
        instr_arg_to_str(output, &instr->args.elems[i]);
    }

    DynamicStr_append(output, "\n");

}

static void basic_block_to_str(struct DynamicStr *output,
        const struct IRBasicBlock *block) {

    u32 i;

    for (i = 0; i < block->instrs.size; i++) {
        instr_to_str(output, &block->instrs.elems[i]);
    }

}

/* if the original C version of func was structured like this:
 *    int foo(int x, unsigned char y, short *z);
 * this function would return:
 *    "i32 x, u8 y, i16* z"
 */
static char* func_args_to_str(const struct IRFunc *func) {

    u32 i;

    struct DynamicStr final_str = DynamicStr_init();

    for (i = 0; i < func->args.size; i++) {

        char *type_str = IR_data_type_to_str(&func->args.elems[i].type);

        DynamicStr_append_printf(&final_str, "%s %s",
                type_str, func->args.elems[i].name);

        m_free(type_str);

        if (i+1 < func->args.size)
            DynamicStr_append(&final_str, ", ");

    }

    return final_str.str;

}

static void func_to_str(struct DynamicStr *output, const struct IRFunc *func) {

    u32 i;

    char *func_type_str = IR_data_type_to_str(&func->ret_type);
    char *func_args_str = func_args_to_str(func);

    DynamicStr_append_printf(output,
            "define %s @%s((%s)) {\n",
            func_type_str, func->name, func_args_str);

    for (i = 0; i < func->blocks.size; i++) {
        DynamicStr_append_printf(output,
                "; start of block %u.\n", i);

        basic_block_to_str(output, &func->blocks.elems[i]);

        DynamicStr_append_printf(output,
                "; end of block %u.\n", i);
    }

    DynamicStr_append(output, "}\n");

    m_free(func_type_str);
    m_free(func_args_str);

}

static void module_to_str(struct DynamicStr *output,
        const struct IRModule *module) {

    u32 i;

    for (i = 0; i < module->funcs.size; i++) {
        func_to_str(output, &module->funcs.elems[i]);
        DynamicStr_append(output, "\n");
    }

}

char* IRToStr_gen(const struct IRModule *module) {

    struct DynamicStr output = DynamicStr_init();

    module_to_str(&output, module);

    return output.str;

}
