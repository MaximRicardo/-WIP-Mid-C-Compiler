#include "code_gen.h"
#include "../../utils/dyn_str.h"
#include "instrs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

u32 sp;

static const char *size_specs[] = {
    "ILLEGAL SIZE",     /* 0 bytes */
    "byte",             /* 1 byte */
    "word",             /* 2 bytes */
    "ILLEGAL SIZE",     /* 3 bytes */
    "dword",            /* 4 bytes */
};

static const char* vreg_to_preg(const char *vreg) {

    /* vregs will be in this format '__eax', '__ebx', etc. */

    assert(strncmp(vreg, "__", 2) == 0);

    return &vreg[2];

}

static bool reg_is_stack_offset(const char *reg) {

    return strncmp(reg, "esp(", 4) == 0 || strncmp(reg, "__esp(", 6) == 0;

}

/* converts stack offset access to NASM style syntax. offset is the preg.
 *    esp(x) -> [esp+index*index_width+x]
 * index_reg, index             - if index_reg is NULL, index will be used as
 *                                the index to the mem access.
 * offset                       - the 'esp(x)' string, NOT just the 'x' part.
 */
static void emit_stack_offset_to_nasm(struct DynamicStr *output,
        const char *offset, const char *index_reg, u32 index,
        u32 index_width, bool add_brackets) {

    u32 offset_val;

    assert(strncmp(offset, "esp(", 4) == 0);

    offset_val = strtoul(&offset[4], NULL, 0);

    if (add_brackets)
        DynamicStr_append(output, "[");

    if (index_reg) {
        DynamicStr_append_printf(output, "esp+%s*%u+%u",
                index_reg, index_width, offset_val);
    }
    else {
        DynamicStr_append_printf(output, "esp+%u*%u+%u",
                index, index_width, offset_val);
    }

    if (add_brackets)
        DynamicStr_append(output, "]");

}

static void emit_instr_arg(struct DynamicStr *output,
        const struct IRInstrArg *arg) {

    if (arg->type == IRInstrArg_REG) {
        if (reg_is_stack_offset(arg->value.reg_name)) {
            emit_stack_offset_to_nasm(output,
                    vreg_to_preg(arg->value.reg_name), NULL, 0, 1, true);
        }
        else {
            DynamicStr_append(output, vreg_to_preg(arg->value.reg_name));
        }
    }
    else if (arg->type == IRInstrArg_IMM32 && arg->data_type.is_signed) {
        DynamicStr_append_printf(output, "%s %d",
                size_specs[arg->data_type.width/8], arg->value.imm_i32);
    }
    else if (arg->type == IRInstrArg_IMM32 && !arg->data_type.is_signed) {
        DynamicStr_append_printf(output, "%s %u",
                size_specs[arg->data_type.width/8], arg->value.imm_u32);
    }
    else if (arg->type == IRInstrArg_STR) {
        DynamicStr_append(output, arg->value.generic_str);
    }
    else {
        assert(false);
    }

}

static void emit_mem_instr_address(struct DynamicStr *output,
        const struct IRInstr *instr) {

    assert(instr->args.size == 3);

    if (instr->args.elems[Arg_LHS].type == IRInstrArg_REG &&
            reg_is_stack_offset(instr->args.elems[Arg_LHS].value.reg_name)) {
        const char *offset = vreg_to_preg(
                instr->args.elems[Arg_LHS].value.reg_name
                );

        emit_stack_offset_to_nasm(output, offset,
                instr->args.elems[Arg_RHS].type == IRInstrArg_REG ?
                instr->args.elems[Arg_RHS].value.reg_name : NULL,
                instr->args.elems[Arg_RHS].value.imm_u32, 1, true);
    }
    else {
        DynamicStr_append(output, "[");
        emit_instr_arg(output, &instr->args.elems[Arg_LHS]);
        DynamicStr_append(output, "+");
        emit_instr_arg(output, &instr->args.elems[Arg_RHS]);
        DynamicStr_append(output, "]");
    }

}

static void gen_from_mov_instr(struct DynamicStr *output,
        const struct IRInstr *instr) {

    assert(instr->args.size == 2);

    DynamicStr_append_printf(output, "mov ");
    emit_instr_arg(output, &instr->args.elems[Arg_SELF]);
    DynamicStr_append(output, ", ");
    emit_instr_arg(output, &instr->args.elems[Arg_LHS]);
    DynamicStr_append(output, "\n");

}

static void gen_from_bin_op(struct DynamicStr *output,
        const struct IRInstr *instr) {

    assert(instr->args.size == 3);
    assert(instr->args.elems[Arg_SELF].type == IRInstrArg_REG);

    /* since x86 is a 2 operand architecture, but MCCIR is a 3 operand IL,
     * instructions will need to be converted from 3 to 2 operands. The
     * conversion looks like this:
     *    add r0 <- r1, r2
     * becomes:
     *    mov r0 <- r1
     *    add r1 <- r2
     */
    DynamicStr_append_printf(output, "mov ");
    emit_instr_arg(output, &instr->args.elems[Arg_SELF]);
    DynamicStr_append(output, ", ");
    emit_instr_arg(output, &instr->args.elems[Arg_LHS]);
    DynamicStr_append(output, "\n");

    DynamicStr_append_printf(output, "%s ",
            X86_get_instr(instr->type, IRInstr_data_type(instr)));
    emit_instr_arg(output, &instr->args.elems[Arg_SELF]);
    DynamicStr_append(output, ", ");
    emit_instr_arg(output, &instr->args.elems[Arg_RHS]);

    DynamicStr_append(output, "\n");

}

/* fuck you intel */
static void gen_from_div_op(struct DynamicStr *output,
        const struct IRInstr *instr) {

    const char *self_reg;
    const char *rhs_reg;
    bool pushed_reg = false;
    bool self_is_eax;

    assert(instr->args.size == 3);
    assert(instr->args.elems[Arg_SELF].type == IRInstrArg_REG);

    self_reg = vreg_to_preg(instr->args.elems[Arg_SELF].value.reg_name);

    self_is_eax =
        strcmp(vreg_to_preg(instr->args.elems[Arg_SELF].value.reg_name),
                "eax") == 0;

    if (!self_is_eax)
        DynamicStr_append(output, "push eax\n");

    if ((instr->args.elems[Arg_LHS].type == IRInstrArg_REG &&
            strcmp(vreg_to_preg(instr->args.elems[Arg_LHS].value.reg_name),
                "eax") != 0) || self_is_eax) {
        DynamicStr_append(output, "mov eax, ");
        emit_instr_arg(output, &instr->args.elems[Arg_LHS]);
        DynamicStr_append(output, "\n");
    }

    if (instr->args.elems[Arg_RHS].type == IRInstrArg_REG &&
            strcmp(vreg_to_preg(instr->args.elems[Arg_RHS].value.reg_name),
                "edx") != 0) {
        rhs_reg = vreg_to_preg(instr->args.elems[Arg_RHS].value.reg_name);
    }
    else {
        pushed_reg = true;
        rhs_reg = strcmp(self_reg, "ebx") == 0 ? "ebx" : "ecx";
        DynamicStr_append_printf(output, "push %s\n", rhs_reg);
        DynamicStr_append_printf(output, "mov %s, ", rhs_reg);
        emit_instr_arg(output, &instr->args.elems[Arg_RHS]);
        DynamicStr_append(output, "\n");
    }

    DynamicStr_append(output, "push edx\n");
    if (IRInstr_data_type(instr).is_signed)
        DynamicStr_append(output, "cdq\n");
    else
        DynamicStr_append(output, "xor edx, edx\n");

    DynamicStr_append_printf(output, "%s %s\n",
            X86_get_instr(instr->type, IRInstr_data_type(instr)), rhs_reg);

    DynamicStr_append(output, "pop edx\n");

    if (pushed_reg) {
        DynamicStr_append_printf(output, "pop %s\n", rhs_reg);
    }

    if (!self_is_eax)
        DynamicStr_append_printf(output, "mov %s, eax\npop eax\n", self_reg);

}

static void gen_from_store_instr(struct DynamicStr *output,
        const struct IRInstr *instr) {

    assert(instr->args.size == 3);

    DynamicStr_append(output, "mov ");
    emit_mem_instr_address(output, instr);
    DynamicStr_append(output, ", ");
    emit_instr_arg(output, &instr->args.elems[Arg_SELF]);
    DynamicStr_append(output, "\n");

}

static void gen_from_load_instr(struct DynamicStr *output,
        const struct IRInstr *instr) {

    assert(instr->args.size == 3);

    DynamicStr_append(output, "mov ");
    emit_instr_arg(output, &instr->args.elems[Arg_SELF]);
    DynamicStr_append(output, ", ");
    emit_mem_instr_address(output, instr);
    DynamicStr_append(output, "\n");

}

static void gen_cmp_instr(struct DynamicStr *output,
        const struct IRInstrArg *cmp_lhs, const struct IRInstrArg *cmp_rhs) {

    bool pushed_eax = cmp_lhs->type != IRInstrArg_REG &&
            cmp_rhs->type != IRInstrArg_REG;

    if (pushed_eax) {
        DynamicStr_append(output, "push eax\nmov eax, ");
        emit_instr_arg(output, cmp_lhs);
        DynamicStr_append(output, "\n");
        DynamicStr_append(output, "cmp eax");
    }
    else {
        DynamicStr_append(output, "cmp ");
        emit_instr_arg(output, cmp_lhs);
    }

    DynamicStr_append(output, ", ");
    emit_instr_arg(output, cmp_rhs);
    DynamicStr_append(output, "\n");

    if (pushed_eax) {
        DynamicStr_append(output, "pop eax\n");
    }

}

static void gen_from_cond_jmp(struct DynamicStr *output,
        const struct IRInstr *instr) {

    assert(instr->args.size == 4);

    gen_cmp_instr(output, &instr->args.elems[0], &instr->args.elems[1]);
    DynamicStr_append_printf(output, "%s .%s\njmp .%s\n",
            X86_get_instr(instr->type, IRInstr_data_type(instr)),
            instr->args.elems[2].value.generic_str,
            instr->args.elems[3].value.generic_str);

}

static void gen_from_uncond_jmp(struct DynamicStr *output,
        const struct IRInstr *instr) {

    assert(instr->args.size == 1);

    DynamicStr_append_printf(output, "jmp .%s\n",
            instr->args.elems[Arg_SELF].value.generic_str);

}

static void gen_from_ret_instr(struct DynamicStr *output,
        const struct IRInstr *instr) {

    assert(instr->args.size == 1);

    DynamicStr_append(output, "mov eax, ");
    emit_instr_arg(output, &instr->args.elems[Arg_SELF]);
    DynamicStr_append(output, "\n");

    DynamicStr_append_printf(output, "sub esp, %d\nret\n", sp);

}

static void gen_x86_from_instr(struct DynamicStr *output,
        const struct IRInstr *instr) {

    /* important that this comes before IRInstrType_is_bin_op */
    if (instr->type == IRInstr_DIV) {
        gen_from_div_op(output, instr);
    }
    else if (IRInstrType_is_bin_op(instr->type)) {
        gen_from_bin_op(output, instr);
    }
    else if (instr->type == IRInstr_ALLOCA) {
        /* the stack size of this function has already been calculated */
    }
    else if (instr->type == IRInstr_STORE) {
        gen_from_store_instr(output, instr);
    }
    else if (instr->type == IRInstr_LOAD) {
        gen_from_load_instr(output, instr);
    }
    else if (IRInstrType_is_cond_branch(instr->type)) {
        gen_from_cond_jmp(output, instr);
    }
    else if (instr->type == IRInstr_JMP) {
        gen_from_uncond_jmp(output, instr);
    }
    else if (instr->type == IRInstr_RET) {
        gen_from_ret_instr(output, instr);
    }
    else if (instr->type == IRInstr_MOV) {
        gen_from_mov_instr(output, instr);
    }
    else {
        assert(false);
    }

}

static void gen_x86_from_block(struct DynamicStr *output,
        const struct IRBasicBlock *block) {

    u32 i;

    DynamicStr_append_printf(output, ".%s:\n", block->label);

    for (i = 0; i < block->instrs.size; i++) {

        gen_x86_from_instr(output, &block->instrs.elems[i]);

    }

}

static void gen_x86_from_func(struct DynamicStr *output,
        const struct IRFunc *func) {

    u32 i;
    u32 func_stack_size = IRFunc_get_stack_size(func);

    DynamicStr_append_printf(output, "%s:\n", func->name);

    sp = 0 - func_stack_size;
    DynamicStr_append_printf(output, "sub esp, %u\n", func_stack_size);

    for (i = 0; i < func->blocks.size; i++) {

        gen_x86_from_block(output, &func->blocks.elems[i]);

    }

    /* in case there was no explicit return */
    DynamicStr_append_printf(output, "sub esp, %d\nret\n", sp);
    sp = 0;

}

char* gen_x86_from_ir(const struct IRModule *module) {

    u32 i;

    struct DynamicStr output = DynamicStr_init();

    DynamicStr_append(&output, "[BITS 32]\n");

    DynamicStr_append(&output, "\nsection .text\n");

    DynamicStr_append(&output, "\nglobal main\n\n");

    for (i = 0; i < module->funcs.size; i++) {
        gen_x86_from_func(&output, &module->funcs.elems[i]);
    }

    return output.str;

}
