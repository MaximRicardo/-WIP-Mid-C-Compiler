#include "alloca_to_esp.h"
#include "../../utils/dyn_str.h"
#include "../../utils/make_str_cpy.h"

static u32 round_up(u32 num, u32 multiple) {

    u32 remainder;

    if (multiple == 0)
        return num;

    remainder = num % multiple;
    if (remainder == 0)
        return num;

    return num+multiple-remainder;

}

static void get_func_stack_size_block(const struct IRBasicBlock *block,
        u32 *stack_size) {

    u32 i;

    for (i = 0; i < block->instrs.size; i++) {

        struct IRInstr *instr = &block->instrs.elems[i];

        if (instr->type != IRInstr_ALLOCA)
            continue;

        *stack_size =
            round_up(*stack_size, instr->args.elems[Arg_RHS].value.imm_u32);

        *stack_size += instr->args.elems[Arg_LHS].value.imm_u32;

    }

}

static u32 get_func_stack_size(const struct IRFunc *func) {

    u32 i;
    u32 stack_size = 0;

    for (i = 0; i < func->blocks.size; i++) {

        get_func_stack_size_block(&func->blocks.elems[i], &stack_size);

    }

    return stack_size;

}

static void alloca_to_esp_instr(struct IRInstr *instr, u32 func_stack_size,
        u32 *n_allocd_bytes, struct IRFunc *parent) {

    u32 sp_offset;
    struct DynamicStr sp_name;
    u32 none_reg_idx;

    if (instr->type != IRInstr_ALLOCA)
        return;

    sp_name = DynamicStr_init();

    /* the allocation has to be processed first to get the correct esp
     * offset */
    *n_allocd_bytes = round_up(*n_allocd_bytes,
            instr->args.elems[Arg_RHS].value.imm_u32);
    *n_allocd_bytes += instr->args.elems[Arg_LHS].value.imm_u32;

    sp_offset = func_stack_size-*n_allocd_bytes;

    DynamicStr_append_printf(&sp_name, "__esp(%u)", sp_offset);

    /* NOTE: ownership of sp_name.str gets passed to parent */
    IRFunc_rename_vreg(parent, instr->args.elems[Arg_SELF].value.reg_name,
            sp_name.str);

    none_reg_idx = StringList_find(&parent->vregs, "__none");
    if (none_reg_idx == m_u32_max) {
        StringList_push_back(&parent->vregs, make_str_copy("__none"));
        none_reg_idx = parent->vregs.size-1;
    }

    instr->args.elems[Arg_SELF].value.reg_name =
        parent->vregs.elems[none_reg_idx];

}

static void alloca_to_esp_block(struct IRBasicBlock *block,
        u32 func_stack_size, u32 *n_allocd_bytes, struct IRFunc *parent) {

    u32 i;

    for (i = 0; i < block->instrs.size; i++) {

        alloca_to_esp_instr(&block->instrs.elems[i], func_stack_size,
                n_allocd_bytes, parent);

    }

}

static void alloca_to_esp_func(struct IRFunc *func) {

    u32 i;

    u32 stack_size = get_func_stack_size(func);
    u32 n_allocd_bytes = 0;

    for (i = 0; i < func->blocks.size; i++) {

        alloca_to_esp_block(&func->blocks.elems[i], stack_size,
                &n_allocd_bytes, func);

    }

}

void X86_alloca_to_esp(struct IRModule *module) {

    u32 i;

    for (i = 0; i < module->funcs.size; i++) {
        alloca_to_esp_func(&module->funcs.elems[i]);
    }

}
