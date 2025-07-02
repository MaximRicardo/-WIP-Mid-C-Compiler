#include "code_gen.h"
#include "../../utils/dyn_str.h"
#include "get_changed_regs.h"
#include "instrs.h"
#include "../../utils/macros.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* TODO:
 *    move the pass to push callee saved registers into another file.
 */

#define m_push_reg(reg) \
    do { \
        DynamicStr_append_printf(output, "push %s\n", reg); \
        n_pushed_bytes += 4; \
    } while (0)

#define m_pop_reg(reg) \
    do { \
        DynamicStr_append_printf(output, "pop %s\n", reg); \
        n_pushed_bytes -= 4; \
    } while (0)

u32 n_pushed_bytes;

static const char *size_specs[] = {
    "ILLEGAL SIZE",     /* 0 bytes */
    "byte",             /* 1 byte */
    "word",             /* 2 bytes */
    "ILLEGAL SIZE",     /* 3 bytes */
    "dword",            /* 4 bytes */
};

static const char *callee_saved_vregs[] = {
    "__ebx",
    "__esi",
    "__edi",
    "__ebp",
};

static const char *pregs[][3] = {

    {"al", "ax", "eax"},
    {"bl", "bx", "ebx"},
    {"cl", "cx", "ecx"},
    {"dl", "dx", "edx"},
    {"INVALID P_REG", "bp", "ebp"},
    {"INVALID P_REG", "si", "esi"},
    {"INVALID P_REG", "di", "edi"},

};

/* eax -> al, cx -> cl, edx -> dl, etc. */
static const char *byte_preg(const char *preg) {

    u32 i;

    for (i = 0; i < m_arr_size(pregs); i++) {
        u32 j;

        for (j = 0; j < m_arr_size(pregs[0]); j++) {
            if (strcmp(preg, pregs[i][j]) == 0)
                return pregs[i][0];
        }
    }

    return NULL;

}

static char str_last_c(const char *str) {

    u32 i = 0;
    while (str[i++] != '\0');

    assert(str[i-1] == '\0');
    return str[i-2];

}

/* stuff like __eax, __ebx, etc. */
static bool is_virt_preg(const char *vreg) {

    return strncmp(vreg, "__", 2) == 0;

}

static const char* vreg_to_preg(const char *vreg) {

    assert(is_virt_preg(vreg));

    return &vreg[2];

}

static bool reg_is_stack_offset(const char *reg) {

    return strncmp(reg, "esp(", 4) == 0 || strncmp(reg, "__esp(", 6) == 0;

}

static bool reg_is_stack_offset_ref(const char *reg) {

    return reg_is_stack_offset(reg) && str_last_c(reg) == '&';

}

/* offset should be in either esp(x) or esp(x)& format */
static u32 get_stack_offset_value(const char *offset) {

    return strtoul(&offset[4], NULL, 0);

}

static char* create_esp_offset(u32 offset, bool is_reference, bool make_vreg) {

    struct DynamicStr str = DynamicStr_init();

    if (make_vreg)
        DynamicStr_append(&str, "__");
    DynamicStr_append_printf(&str, "esp(%u)", offset);
    if (is_reference)
        DynamicStr_append(&str, "&");

    return str.str;

}

/* pushing the callee saved registers on the stack offsets the stack pointer by
 * an amount not accounted for in earlier passes, so we gotta account for it
 * here. this has no effect on the offset of stack variables, but it does have
 * an effect on the offset of the function arguments. */
static void args_account_for_saved_regs(u32 n_pushed, struct IRFunc *func) {

    u32 i;

    u32 stack_size = IRFunc_get_stack_size(func);

    for (i = 0; i < func->vregs.size; i++) {

        char *vreg = func->vregs.elems[i];
        char *new_vreg = NULL;
        const char *preg = NULL;
        u32 offset;
        bool is_ref;

        if (!is_virt_preg(vreg) || !reg_is_stack_offset(vreg))
            continue;

        preg = vreg_to_preg(vreg);

        offset = get_stack_offset_value(preg);
        is_ref = reg_is_stack_offset_ref(preg);

        /* if the offset doesn't reach outside the function's frame, it's
         * accessing a local var and not an argument */
        if (offset < stack_size)
            continue;

        new_vreg = create_esp_offset(offset+n_pushed*4, is_ref, true);
        IRFunc_rename_vreg(func, vreg, new_vreg);

    }

}

static void push_callee_saved_regs(struct DynamicStr *output,
        struct IRFunc *func) {

    u32 i;
    u32 n_pushed = 0;
    struct ConstStringList vregs = X86_func_get_changed_vregs(func);

    for (i = 0; i < m_arr_size(callee_saved_vregs); i++) {
        const char *preg = NULL;

        if (ConstStringList_find(&vregs, callee_saved_vregs[i]) == m_u32_max)
            continue;

        preg = vreg_to_preg(callee_saved_vregs[i]);
        DynamicStr_append_printf(output, "push %s\n", preg);
        ++n_pushed;
    }

    ConstStringList_free(&vregs);

    args_account_for_saved_regs(n_pushed, func);

}

static void pop_callee_saved_regs(struct DynamicStr *output,
        const struct IRFunc *func) {

    u32 i;
    u32 n = m_arr_size(callee_saved_vregs);
    struct ConstStringList vregs = X86_func_get_changed_vregs(func);

    for (i = n-1; i < n; i--) {
        const char *preg = NULL;

        if (ConstStringList_find(&vregs, callee_saved_vregs[i]) == m_u32_max)
            continue;

        preg = vreg_to_preg(callee_saved_vregs[i]);
        DynamicStr_append_printf(output, "pop %s\n", preg);
    }

    ConstStringList_free(&vregs);

}

/* increments n pushed bytes by 12 */
static void push_caller_saved_regs(struct DynamicStr *output) {

    m_push_reg("eax");
    m_push_reg("ecx");
    m_push_reg("edx");

}

/* decrements n pushed bytes by 12 */
static void pop_caller_saved_regs(struct DynamicStr *output) {

    m_pop_reg("edx");
    m_pop_reg("ecx");
    m_pop_reg("eax");

}

/* converts stack offset access to NASM style syntax. offset is the preg.
 *    esp(x) -> [esp+index*index_width+x]
 *
 * index_reg, index             - if index_reg is NULL, index will be used as
 *                                the index to the mem access.
 * offset                       - the 'esp(x)' string, NOT just the 'x' part.
 */
static void emit_stack_offset_to_nasm(struct DynamicStr *output,
        const char *offset, const char *index_reg, u32 index,
        u32 index_width) {

    u32 offset_val;

    assert(strncmp(offset, "esp(", 4) == 0);

    offset_val = get_stack_offset_value(offset);

    if (index_reg) {
        DynamicStr_append_printf(output, "[esp+%s*%u+%u]",
                index_reg, index_width, offset_val);
    }
    else {
        DynamicStr_append_printf(output, "[esp+%u*%u+%u]",
                index, index_width, offset_val);
    }

}

static void emit_instr_arg(struct DynamicStr *output,
        const struct IRInstrArg *arg, bool emit_size_spec) {

    if (arg->type == IRInstrArg_REG) {
        if (reg_is_stack_offset(arg->value.reg_name)) {
            emit_stack_offset_to_nasm(output,
                    vreg_to_preg(arg->value.reg_name), NULL, n_pushed_bytes,
                    1);
        }
        else {
            DynamicStr_append(output, vreg_to_preg(arg->value.reg_name));
        }
    }
    else if (arg->type == IRInstrArg_IMM32) {
        if (emit_size_spec) {
            DynamicStr_append_printf(output, "%s ",
                    size_specs[arg->data_type.width/8]);
        }

        if (arg->data_type.is_signed)
            DynamicStr_append_printf(output, "%d", arg->value.imm_i32);
        else
            DynamicStr_append_printf(output, "%u", arg->value.imm_u32);
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

    const struct IRInstrArg *lhs_arg = NULL;
    const struct IRInstrArg *rhs_arg = NULL;

    assert(instr->args.size == 3);

    lhs_arg = &instr->args.elems[Arg_LHS];
    rhs_arg = &instr->args.elems[Arg_RHS];

    if (lhs_arg->type == IRInstrArg_REG &&
            reg_is_stack_offset(lhs_arg->value.reg_name)) {
        const char *offset = vreg_to_preg(lhs_arg->value.reg_name);

        emit_stack_offset_to_nasm(output, offset,
                rhs_arg->type == IRInstrArg_REG ?
                    rhs_arg->value.reg_name : NULL,
                rhs_arg->value.imm_u32, 1);
    }
    else {
        DynamicStr_append(output, "[");
        emit_instr_arg(output, lhs_arg, false);
        DynamicStr_append(output, "+");
        emit_instr_arg(output, rhs_arg, false);
        DynamicStr_append(output, "]");
    }

}

static void gen_from_mov_instr(struct DynamicStr *output,
        const struct IRInstr *instr) {

    const struct IRInstrArg *self_arg = NULL;
    const struct IRInstrArg *lhs_arg = NULL;

    bool lhs_on_stack;
    bool use_lea;
    const char *lhs_temp_reg = NULL;

    assert(instr->args.size == 2);
    assert(instr->args.elems[Arg_SELF].type == IRInstrArg_REG);

    self_arg = &instr->args.elems[Arg_SELF];
    lhs_arg = &instr->args.elems[Arg_LHS];

    lhs_on_stack = lhs_arg->type == IRInstrArg_REG &&
        reg_is_stack_offset(lhs_arg->value.reg_name);

    use_lea = lhs_on_stack && reg_is_stack_offset_ref(lhs_arg->value.reg_name);

    if (use_lea) {

        bool pushed_reg = false;
        if (reg_is_stack_offset(self_arg->value.reg_name)) {
            m_push_reg("eax");
            pushed_reg = true;
        }

        DynamicStr_append_printf(output, "lea %s, ", pushed_reg ?
                "eax" : vreg_to_preg(self_arg->value.reg_name));
        emit_instr_arg(output, lhs_arg, true);
        DynamicStr_append(output, "\n");

        if (pushed_reg) {
            m_pop_reg("eax");
        }

    }
    else {

        if (lhs_on_stack) {

            lhs_temp_reg = strcmp(
                    vreg_to_preg(self_arg->value.reg_name), "eax"
                    ) == 0 ? "ebx" : "eax";

            m_push_reg(lhs_temp_reg);

            DynamicStr_append_printf(output, "mov %s, ", lhs_temp_reg);
            emit_instr_arg(output, lhs_arg, true);
            DynamicStr_append(output, "\n");

        }

        DynamicStr_append_printf(output, "mov ");

        emit_instr_arg(output, self_arg, true);

        DynamicStr_append(output, ", ");

        if (lhs_on_stack) {
            DynamicStr_append(output, lhs_temp_reg);
        }
        else {
            emit_instr_arg(output, lhs_arg, true);
        }

        DynamicStr_append(output, "\n");

        if (lhs_on_stack) {
            m_pop_reg(lhs_temp_reg);
        }

    }

}

static void gen_from_bin_op(struct DynamicStr *output,
        const struct IRInstr *instr) {

    const struct IRInstrArg *self_arg = NULL;
    const struct IRInstrArg *lhs_arg = NULL;
    const struct IRInstrArg *rhs_arg = NULL;

    bool self_on_stack = false;

    assert(instr->args.size == 3);
    assert(instr->args.elems[Arg_SELF].type == IRInstrArg_REG);

    self_arg = &instr->args.elems[Arg_SELF];
    lhs_arg = &instr->args.elems[Arg_LHS];
    rhs_arg = &instr->args.elems[Arg_RHS];

    self_on_stack = reg_is_stack_offset(
            vreg_to_preg(self_arg->value.reg_name)
            );

    if (self_on_stack) {
        /* blame intel, not me. */

        m_push_reg("eax");

        if (lhs_arg->type != IRInstrArg_REG ||
                strcmp(lhs_arg->value.reg_name, "__eax") != 0) {

            DynamicStr_append(output, "mov eax, ");
            emit_instr_arg(output, lhs_arg, true);
            DynamicStr_append(output, "\n");

        }

        DynamicStr_append_printf(output, "%s eax, ",
                X86_get_instr(instr->type, IRInstr_data_type(instr)));
        emit_instr_arg(output, rhs_arg, true);
        DynamicStr_append(output, "\n");

        /* remember to move the result from pushed_reg to the actual dest
         * reg */
        DynamicStr_append(output, "mov ");
        emit_instr_arg(output, self_arg, true);
        DynamicStr_append(output, ", eax\n");

        m_pop_reg("eax");
    }
    else {
        /* since x86 is a 2 operand architecture, but MCCIR is a 3 operand IL,
         * instructions will need to be converted from 3 to 2 operands. The
         * conversion looks like this:
         *    add r0 <- r1, r2
         * becomes:
         *    mov r0 <- r1
         *    add r1 <- r2
         */
        DynamicStr_append_printf(output, "mov ");
        emit_instr_arg(output, self_arg, true);
        DynamicStr_append(output, ", ");
        emit_instr_arg(output, lhs_arg, true);
        DynamicStr_append(output, "\n");

        DynamicStr_append_printf(output, "%s ",
                X86_get_instr(instr->type, IRInstr_data_type(instr)));
        emit_instr_arg(output, self_arg, true);
        DynamicStr_append(output, ", ");
        emit_instr_arg(output, rhs_arg, true);
        DynamicStr_append(output, "\n");
    }

}

/* fuck you intel */
static void gen_from_div_op(struct DynamicStr *output,
        const struct IRInstr *instr) {

    const char *self_reg = NULL;
    const char *rhs_reg = NULL;
    const struct IRInstrArg *self_arg = NULL;
    const struct IRInstrArg *lhs_arg = NULL;
    const struct IRInstrArg *rhs_arg = NULL;
    bool pushed_reg = false;
    bool self_is_eax;

    assert(instr->args.size == 3);
    assert(instr->args.elems[Arg_SELF].type == IRInstrArg_REG);

    self_arg = &instr->args.elems[Arg_SELF];
    lhs_arg = &instr->args.elems[Arg_LHS];
    rhs_arg = &instr->args.elems[Arg_RHS];

    self_reg = vreg_to_preg(self_arg->value.reg_name);

    self_is_eax =
        strcmp(vreg_to_preg(self_arg->value.reg_name), "eax") == 0;

    if (!self_is_eax)
        m_push_reg("eax");

    if (lhs_arg->type != IRInstrArg_REG ||
            strcmp(lhs_arg->value.reg_name, "__eax") != 0) {
        DynamicStr_append(output, "mov eax, ");
        emit_instr_arg(output, lhs_arg, true);
        DynamicStr_append(output, "\n");
    }

    if (rhs_arg->type == IRInstrArg_REG &&
            strcmp(vreg_to_preg(rhs_arg->value.reg_name), "edx") != 0 &&
            !reg_is_stack_offset(vreg_to_preg(rhs_arg->value.reg_name))) {
        rhs_reg = vreg_to_preg(rhs_arg->value.reg_name);
    }
    else {
        pushed_reg = true;
        rhs_reg = strcmp(self_reg, "ebx") == 0 ? "ebx" : "ecx";
        m_push_reg(rhs_reg);
        DynamicStr_append_printf(output, "mov %s, ", rhs_reg);
        emit_instr_arg(output, rhs_arg, true);
        DynamicStr_append(output, "\n");
    }

    m_push_reg("edx");
    if (IRInstr_data_type(instr).is_signed)
        DynamicStr_append(output, "cdq\n");
    else
        DynamicStr_append(output, "xor edx, edx\n");

    DynamicStr_append_printf(output, "%s %s\n",
            X86_get_instr(instr->type, IRInstr_data_type(instr)), rhs_reg);

    m_pop_reg("edx");

    if (pushed_reg) {
        m_pop_reg(rhs_reg);
    }

    if (!self_is_eax) {
        DynamicStr_append(output, "mov ");
        emit_instr_arg(output, self_arg, true);
        DynamicStr_append(output, ", eax\n");
        m_pop_reg("eax");
    }

}

static void gen_from_store_instr(struct DynamicStr *output,
        const struct IRInstr *instr) {

    assert(instr->args.size == 3);

    DynamicStr_append(output, "mov ");
    emit_mem_instr_address(output, instr);
    DynamicStr_append(output, ", ");
    emit_instr_arg(output, &instr->args.elems[Arg_SELF], true);
    DynamicStr_append(output, "\n");

}

static void gen_from_load_instr(struct DynamicStr *output,
        const struct IRInstr *instr) {

    assert(instr->args.size == 3);

    DynamicStr_append(output, "mov ");
    emit_instr_arg(output, &instr->args.elems[Arg_SELF], true);
    DynamicStr_append(output, ", ");
    emit_mem_instr_address(output, instr);
    DynamicStr_append(output, "\n");

}

static void gen_cmp_instr(struct DynamicStr *output,
        const struct IRInstrArg *cmp_lhs, const struct IRInstrArg *cmp_rhs) {

    bool pushed_eax = cmp_lhs->type != IRInstrArg_REG &&
            cmp_rhs->type != IRInstrArg_REG;

    if (pushed_eax) {
        m_push_reg("eax");
        DynamicStr_append(output, "mov eax, ");
        emit_instr_arg(output, cmp_lhs, true);
        DynamicStr_append(output, "\n");
        DynamicStr_append(output, "cmp eax");
    }
    else {
        DynamicStr_append(output, "cmp ");
        emit_instr_arg(output, cmp_lhs, true);
    }

    DynamicStr_append(output, ", ");
    emit_instr_arg(output, cmp_rhs, true);
    DynamicStr_append(output, "\n");

    if (pushed_eax) {
        m_pop_reg("eax");
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
        const struct IRInstr *instr, const struct IRFunc *cur_func,
        u32 func_stack_size) {

    const struct IRInstrArg *self_arg = NULL;

    assert(instr->args.size == 1);

    self_arg = &instr->args.elems[Arg_SELF];

    if (self_arg->type != IRInstrArg_REG ||
            strcmp(self_arg->value.reg_name, "__eax") != 0) {
        DynamicStr_append(output, "mov eax, ");
        emit_instr_arg(output, self_arg, true);
        DynamicStr_append(output, "\n");
    }

    assert(n_pushed_bytes == 0);

    if (func_stack_size > 0)
        DynamicStr_append_printf(output, "add esp, %u\n", func_stack_size);
    pop_callee_saved_regs(output, cur_func);
    DynamicStr_append(output, "ret\n");

}

static void gen_from_cmp_oper(struct DynamicStr *output,
        const struct IRInstr *instr) {

    bool pushed_eax = false;

    const struct IRInstrArg *self_arg = NULL;
    const struct IRInstrArg *lhs_arg = NULL;
    const struct IRInstrArg *rhs_arg = NULL;
    const char *b_preg = NULL;

    assert(instr->args.size == 3);
    assert(instr->args.elems[Arg_SELF].type == IRInstrArg_REG);

    self_arg = &instr->args.elems[Arg_SELF];
    lhs_arg = &instr->args.elems[Arg_LHS];
    rhs_arg = &instr->args.elems[Arg_RHS];

    b_preg = vreg_to_preg(self_arg->value.reg_name);
    assert(b_preg);

    if (strcmp(b_preg, "INVALID P_REG") == 0 || strcmp(b_preg, "esp") == 0) {
        pushed_eax = true;
        m_push_reg("eax");
        b_preg = "eax";
    }

    gen_cmp_instr(output, lhs_arg, rhs_arg);

    DynamicStr_append_printf(output, "%s %s\n",
            X86_get_instr(instr->type, IRInstr_data_type(instr)),
            byte_preg(b_preg));

    DynamicStr_append_printf(output, "and %s, 0xff\n",
            b_preg);

    if (pushed_eax) {
        DynamicStr_append_printf(output, "mov ");
        emit_instr_arg(output, self_arg, true);
        DynamicStr_append(output, ", eax\n");
        m_pop_reg("eax");
    }

}

static void gen_from_call_instr(struct DynamicStr *output,
        const struct IRInstr *instr) {

    u32 i;
    u32 arg_bytes = 0;

    assert(instr->args.size >= 1);
    assert(instr->args.elems[Arg_SELF].type == IRInstrArg_STR);

    push_caller_saved_regs(output);

    /* arguments get pushed in right to left order */
    for (i = instr->args.size-1; i >= 2; i--) {

        const struct IRInstrArg *arg = &instr->args.elems[i];

        DynamicStr_append(output, "push ");
        emit_instr_arg(output, arg, true);
        DynamicStr_append(output, "\n");
        /* not good. improve later pls */
        n_pushed_bytes += 4;
        arg_bytes += 4;

    }

    DynamicStr_append_printf(output, "call %s",
            instr->args.elems[Arg_SELF].value.generic_str);
    DynamicStr_append(output, "\n");

    /* clean up */
    DynamicStr_append_printf(output, "add esp, %u\n", arg_bytes);
    n_pushed_bytes -= arg_bytes;
    pop_caller_saved_regs(output);

}

static void gen_x86_from_instr(struct DynamicStr *output,
        const struct IRInstr *instr, const struct IRFunc *cur_func,
        u32 func_stack_size) {

    if (instr->type == IRInstr_DIV) {
        gen_from_div_op(output, instr);
    }
    else if (IRInstrType_is_cmp_op(instr->type)) {
        gen_from_cmp_oper(output, instr);
    }
    /* important that this comes after div and cmp_op */
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
        gen_from_ret_instr(output, instr, cur_func, func_stack_size);
    }
    else if (instr->type == IRInstr_MOV) {
        gen_from_mov_instr(output, instr);
    }
    else if (instr->type == IRInstr_CALL) {
        gen_from_call_instr(output, instr);
    }
    else {
        assert(false);
    }

}

static void gen_x86_from_block(struct DynamicStr *output,
        const struct IRBasicBlock *block, const struct IRFunc *cur_func,
        u32 func_stack_size) {

    u32 i;

    DynamicStr_append_printf(output, ".%s:\n", block->label);

    for (i = 0; i < block->instrs.size; i++) {

        gen_x86_from_instr(output, &block->instrs.elems[i], cur_func,
                func_stack_size);

    }

}

static void gen_ret_if_no_ret_stmt(struct DynamicStr *output,
        const struct IRFunc *func, u32 func_stack_size) {

    const struct IRBasicBlock *last_blck =
        &func->blocks.elems[func->blocks.size-1];
    const struct IRInstr *last_instr =
        &last_blck->instrs.elems[last_blck->instrs.size-1];

    if (last_instr->type == IRInstr_RET)
        return;

    if (func_stack_size > 0)
        DynamicStr_append_printf(output, "add esp, %u\n", func_stack_size);
    pop_callee_saved_regs(output, func);
    /* default func ret value */
    DynamicStr_append(output, "mov eax, 0\n");
    DynamicStr_append(output, "ret\n");

}

static void gen_from_func_mods(struct DynamicStr *output,
        struct IRFunc *func) {

    if (func->mods.is_extern)
        DynamicStr_append_printf(output, "extern %s\n", func->name);
    if (func->mods.is_global)
        DynamicStr_append_printf(output, "global %s\n", func->name);

}

static void gen_x86_from_func(struct DynamicStr *output, struct IRFunc *func) {

    u32 i;
    u32 stack_size = IRFunc_get_stack_size(func);

    gen_from_func_mods(output, func);

    if (!IRFunc_has_body(func))
        return;

    n_pushed_bytes = 0;

    DynamicStr_append_printf(output, "%s:\n", func->name);

    push_callee_saved_regs(output, func);
    if (stack_size > 0)
        DynamicStr_append_printf(output, "sub esp, %u\n", stack_size);

    for (i = 0; i < func->blocks.size; i++) {

        gen_x86_from_block(output, &func->blocks.elems[i], func, stack_size);

    }

    assert(n_pushed_bytes == 0);

    gen_ret_if_no_ret_stmt(output, func, stack_size);

}

char* gen_x86_from_ir(struct IRModule *module) {

    u32 i;

    struct DynamicStr output = DynamicStr_init();

    DynamicStr_append(&output, "[BITS 32]\n");

    DynamicStr_append(&output, "\nsection .text\n\n");

    for (i = 0; i < module->funcs.size; i++) {
        gen_x86_from_func(&output, &module->funcs.elems[i]);
    }

    return output.str;

}
