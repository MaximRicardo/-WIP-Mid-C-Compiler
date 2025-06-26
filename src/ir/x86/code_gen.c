#include "code_gen.h"
#include "../../utils/dyn_str.h"
#include "../../macros.h"
#include "../../utils/make_str_cpy.h"
#include "instrs.h"
#include "reg_lifetimes.h"
#include "phys_reg_val.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

u32 sp;

enum IRInstrArgIndices {

    Arg_SELF = 0,
    Arg_LHS = 1,
    Arg_RHS = 2

};

enum CPURegs {

    CPUReg_AX,
    CPUReg_BX,
    CPUReg_CX,
    CPUReg_DX,
    CPUReg_SP,
    CPUReg_BP,
    CPUReg_SI,
    CPUReg_DI

};

/* must be in the same order as enum CPURegs. */
char *cpu_regs[] = {

    "eax",
    "ebx",
    "ecx",
    "edx",
    "esp",
    "ebp",
    "esi",
    "edi"

};

struct PhysRegVal cpu_reg_vals[m_arr_size(cpu_regs)];

/* if a virtual reg maps to a cpu reg, it's index must be one of these.
 * edx isn't used as a gp reg due to the generational fuck-up that is the div
 * instruction. */
u32 gp_regs[] = {
    CPUReg_AX, CPUReg_BX, CPUReg_CX,  CPUReg_SI, CPUReg_BP
};

static void init_cpu_reg_vals(void) {

    u32 i;

    for (i = 0; i < m_arr_size(cpu_reg_vals); i++) {
        cpu_reg_vals[i] = PhysRegVal_init();
    }

}

static void free_cpu_reg_vals(void) {

    u32 i;

    for (i = 0; i < m_arr_size(cpu_reg_vals); i++) {
        PhysRegVal_free(cpu_reg_vals[i]);
    }

}

ATTRIBUTE((unused))
static void print_cpu_reg_vals(void) {

    u32 i;

    for (i = 0; i < m_arr_size(cpu_reg_vals); i++) {
        printf("%s: %%%s\n", cpu_regs[i], cpu_reg_vals[i].virt_reg);
    }

}

/* returns m_u32_max if every register has already been alloced */
static u32 get_reg_to_alloc(const struct IRRegLTList *vreg_lts,
        u32 cur_instr_idx) {

    u32 i;

    for (i = 0; i < m_arr_size(gp_regs); i++) {
        u32 reg_idx = gp_regs[i];
        u32 cpu_reg_lt_idx;

        if (!cpu_reg_vals[reg_idx].virt_reg) {
            return reg_idx;
        }

        cpu_reg_lt_idx =
            IRRegLTList_find_reg(vreg_lts, cpu_reg_vals[reg_idx].virt_reg);

        if (cur_instr_idx > vreg_lts->elems[cpu_reg_lt_idx].death_idx) {
            return reg_idx;
        }
    }

    return m_u32_max;

}

/* breaks if there are more virtual registers than physical ones */
static u32 virt_reg_to_cpu_reg(const char *virt_reg,
        const struct IRRegLTList *vreg_lts, u32 cur_instr_idx) {

    u32 i;

    /* check if any registers already hold virt_reg before trying to allocate
     * a reg */
    for (i = 0; i < m_arr_size(gp_regs); i++) {
        u32 reg_idx = gp_regs[i];

        if (!cpu_reg_vals[reg_idx].virt_reg)
            continue;

        if (strcmp(cpu_reg_vals[reg_idx].virt_reg, virt_reg) == 0) {
            break;
        }
    }

    if (i >= m_arr_size(gp_regs))
        i = get_reg_to_alloc(vreg_lts, cur_instr_idx);
    else
        i = gp_regs[i];

    if (i == m_u32_max) {
        /* for now just crash if there aren't any free regs left */
        fprintf(stderr, "RAN OUT OF CPU REGISTERS!\n");
        assert(false);
    }

    PhysRegVal_free(cpu_reg_vals[i]);
    cpu_reg_vals[i] = PhysRegVal_create(make_str_copy(virt_reg));

    assert(i != CPUReg_DX);

    return i;

}

static void load_operand_to_reg(struct DynamicStr *output, u32 reg_idx,
        const struct IRInstrArg *operand, const struct IRRegLTList *vreg_lts,
        u32 cur_instr_idx) {

    if (operand->type == IRInstrArg_REG) {
        u32 operand_phys_reg = virt_reg_to_cpu_reg(operand->value.reg_name,
                vreg_lts, cur_instr_idx);

        if(operand_phys_reg != reg_idx) {
            DynamicStr_append_printf(output, "mov %s, %s\n",
                    cpu_regs[reg_idx], cpu_regs[operand_phys_reg]);
        }
        else {
            /* idk whether i should let this slide or not, but so far letting
             * it slide has been a bad idea, so i'ma just put an assert(false)
             * here */
            /*assert(false);*/
        }
    }
    else if (operand->type == IRInstrArg_IMM32 &&
            operand->data_type.is_signed) {
        DynamicStr_append_printf(output, "mov %s, %d\n",
                cpu_regs[reg_idx], operand->value.imm_i32);
    }
    else if (operand->type == IRInstrArg_IMM32 &&
            !operand->data_type.is_signed) {
        DynamicStr_append_printf(output, "mov %s, %u\n",
                cpu_regs[reg_idx], operand->value.imm_u32);
    }
    else {
        assert(false);
    }

}

static void emit_instr_operand(struct DynamicStr *output,
        const struct IRInstrArg *arg, const struct IRRegLTList *vreg_lts,
        u32 cur_instr_idx) {

    if (arg->type == IRInstrArg_REG) {
        u32 operand_phys_reg = virt_reg_to_cpu_reg(arg->value.reg_name,
                vreg_lts, cur_instr_idx);

        DynamicStr_append(output, cpu_regs[operand_phys_reg]);
    }
    else if (arg->type == IRInstrArg_IMM32 && arg->data_type.is_signed) {
        DynamicStr_append_printf(output, "%d", arg->value.imm_i32);
    }
    else if (arg->type == IRInstrArg_IMM32 && !arg->data_type.is_signed) {
        DynamicStr_append_printf(output, "%u", arg->value.imm_u32);
    }
    else if (arg->type == IRInstrArg_STR) {
        DynamicStr_append(output, arg->value.generic_str);
    }
    else {
        assert(false);
    }

}

static void gen_from_bin_instr(struct DynamicStr *output,
        const struct IRInstr *instr, const struct IRRegLTList *vreg_lts,
        u32 cur_instr_idx) {

    u32 self_reg;

    assert(instr->args.size == 3);
    assert(instr->args.elems[Arg_SELF].type == IRInstrArg_REG);

    self_reg = virt_reg_to_cpu_reg(instr->args.elems[Arg_SELF].value.reg_name,
            vreg_lts, cur_instr_idx);

    /* since x86 is a 2 operand architecture, but MCCIR is a 3 operand IL,
     * instructions will need to be converted from 3 to 2 operands. The
     * conversion looks like this:
     *    add r0 <- r1, r2
     * becomes:
     *    mov r0 <- r1
     *    add r1 <- r2
     */
    load_operand_to_reg(output, self_reg, &instr->args.elems[Arg_LHS], vreg_lts,
            cur_instr_idx);

    DynamicStr_append_printf(output, "%s %s, ",
            X86_get_instr(instr->type, IRInstr_data_type(instr)),
            cpu_regs[self_reg]);

    emit_instr_operand(output, &instr->args.elems[Arg_RHS], vreg_lts,
            cur_instr_idx);
    DynamicStr_append(output, "\n");

}

static void gen_from_div_instr(struct DynamicStr *output,
        const struct IRInstr *instr, const struct IRRegLTList *vreg_lts,
        u32 cur_instr_idx) {

    u32 self_reg;
    u32 rhs_reg;

    assert(instr->args.size == 3);
    assert(instr->args.elems[Arg_SELF].type == IRInstrArg_REG);

    /* get the rhs first, in case moving self into ax requires messing with
     * the stack, with would make all the stack offsets off by 4 bytes */
    if (instr->args.elems[Arg_RHS].type == IRInstrArg_REG) {
        rhs_reg = virt_reg_to_cpu_reg(instr->args.elems[2].value.reg_name,
                vreg_lts, cur_instr_idx);
    }
    else if (instr->args.elems[Arg_RHS].type == IRInstrArg_IMM32 &&
            instr->args.elems[Arg_RHS].data_type.is_signed) {
        rhs_reg = CPUReg_DI;
        DynamicStr_append_printf(output, "mov edi, %d\n",
                instr->args.elems[Arg_RHS].value.imm_i32);
    }
    else if (instr->args.elems[Arg_RHS].type == IRInstrArg_IMM32 &&
            instr->args.elems[Arg_RHS].data_type.is_signed) {
        rhs_reg = CPUReg_DI;
        DynamicStr_append_printf(output, "mov edi, %u\n",
                instr->args.elems[Arg_RHS].value.imm_u32);
    }
    else {
        assert(false);
    }

    self_reg = virt_reg_to_cpu_reg(instr->args.elems[Arg_SELF].value.reg_name,
            vreg_lts, cur_instr_idx);
    if (self_reg != CPUReg_AX) {
        DynamicStr_append(output, "push eax\n");
    }
    load_operand_to_reg(output, CPUReg_AX, &instr->args.elems[Arg_LHS],
            vreg_lts, cur_instr_idx);

    /* very important to zero/sign extend into edx before dividing. */
    if (IRInstr_data_type(instr).is_signed)
        DynamicStr_append(output, "cdq\n");
    else
        DynamicStr_append(output, "xor edx, edx\n");
    /* the div could be signed or unsigned, so it's best to use
     * X86_get_instr */
    DynamicStr_append_printf(output, "%s %s\n",
            X86_get_instr(instr->type, IRInstr_data_type(instr)),
            cpu_regs[rhs_reg]);

    if (self_reg != CPUReg_AX) {
        DynamicStr_append_printf(output, "mov %s, eax\n"
                                         "pop eax\n",
                                         cpu_regs[self_reg]);
    }

}

static void gen_cmp_instr(struct DynamicStr *output,
        const struct IRInstrArg *cmp_lhs, const struct IRInstrArg *cmp_rhs,
        const struct IRRegLTList *vreg_lts, u32 cur_instr_idx) {

    if (cmp_lhs->type != IRInstrArg_REG) {
        DynamicStr_append(output, "mov edx, ");
        emit_instr_operand(output, cmp_lhs, vreg_lts, cur_instr_idx);
        DynamicStr_append(output, "\n");
    }

    DynamicStr_append(output, "cmp ");

    if (cmp_lhs->type == IRInstrArg_REG) {
        emit_instr_operand(output, cmp_lhs, vreg_lts, cur_instr_idx);
    }
    else {
        DynamicStr_append(output, "edx");
    }

    DynamicStr_append(output, ", ");
    emit_instr_operand(output, cmp_rhs, vreg_lts, cur_instr_idx);
    DynamicStr_append(output, "\n");

}

static void gen_from_cond_jmp_instr(struct DynamicStr *output,
        const struct IRInstr *instr, const struct IRRegLTList *vreg_lts,
        u32 cur_instr_idx) {

    assert(instr->args.size == 3);
    assert(instr->args.elems[Arg_RHS].type == IRInstrArg_STR);

    /* the conditional jump will get split into a seperate cmp and jmp
     * instruction. */

    gen_cmp_instr(output, &instr->args.elems[Arg_SELF],
            &instr->args.elems[Arg_LHS], vreg_lts, cur_instr_idx);

    DynamicStr_append_printf(output, "%s .%s\n",
            X86_get_instr(instr->type, IRInstr_data_type(instr)),
            instr->args.elems[Arg_RHS].value.generic_str);

}

static void gen_from_uncond_jmp_instr(struct DynamicStr *output,
        const struct IRInstr *instr) {

    assert(instr->args.size == 1);
    assert(instr->args.elems[Arg_SELF].type == IRInstrArg_STR);

    DynamicStr_append_printf(output, "jmp .%s\n",
            instr->args.elems[Arg_SELF].value.generic_str);

}

static void gen_from_ret_instr(struct DynamicStr *output,
        const struct IRInstr *instr, const struct IRRegLTList *vreg_lts,
        u32 cur_instr_idx) {

    load_operand_to_reg(output, CPUReg_AX, &instr->args.elems[Arg_SELF],
            vreg_lts, cur_instr_idx);

    /* esp needs to get sent back to the return address */
    DynamicStr_append_printf(output, "sub esp, %d\n", sp);

    DynamicStr_append(output, "ret\n");

}

static void gen_from_alloca_instr(struct DynamicStr *output,
        const struct IRInstr *instr, const struct IRRegLTList *vreg_lts,
        u32 cur_instr_idx) {

    u32 self_reg;
    u32 amount_to_sub = 0;

    assert(instr->args.size == 3);
    assert(instr->args.elems[Arg_SELF].type == IRInstrArg_REG);
    assert(instr->args.elems[Arg_LHS].type == IRInstrArg_IMM32);
    assert(instr->args.elems[Arg_RHS].type == IRInstrArg_IMM32);

    self_reg = virt_reg_to_cpu_reg(instr->args.elems[Arg_SELF].value.reg_name,
            vreg_lts, cur_instr_idx);

    if (sp % instr->args.elems[Arg_RHS].value.imm_u32) {
        amount_to_sub += sp % instr->args.elems[Arg_RHS].value.imm_u32;
    }
    amount_to_sub += instr->args.elems[Arg_LHS].value.imm_u32;

    DynamicStr_append_printf(output, "sub esp, %u\n", amount_to_sub);
    sp -= amount_to_sub;

    DynamicStr_append_printf(output, "mov %s, esp\n", cpu_regs[self_reg]);

}

static void gen_from_store_instr(struct DynamicStr *output,
        const struct IRInstr *instr, const struct IRRegLTList *vreg_lts,
        u32 cur_instr_idx) {

    u32 self_reg = m_u32_max;
    u32 lhs_reg;

    assert(instr->args.size == 3);
    assert(instr->args.elems[Arg_LHS].type == IRInstrArg_REG);

    if (instr->args.elems[Arg_SELF].type == IRInstrArg_REG) {
        self_reg = virt_reg_to_cpu_reg(
                instr->args.elems[Arg_SELF].value.reg_name, vreg_lts,
                cur_instr_idx
                );
    }

    lhs_reg = virt_reg_to_cpu_reg(instr->args.elems[Arg_LHS].value.reg_name,
            vreg_lts, cur_instr_idx);

    if (instr->args.elems[Arg_RHS].type == IRInstrArg_REG) {
        u32 rhs_reg = virt_reg_to_cpu_reg(
                instr->args.elems[Arg_RHS].value.reg_name, vreg_lts,
                cur_instr_idx);

        DynamicStr_append_printf(output, "mov [%s+%s], ",
                cpu_regs[lhs_reg], cpu_regs[rhs_reg]);
    }
    else {
        DynamicStr_append_printf(output, "mov [%s+%d], ",
                cpu_regs[lhs_reg], instr->args.elems[Arg_RHS].value.imm_i32);
    }

    if (self_reg != m_u32_max) {
        DynamicStr_append_printf(output, "%s\n", cpu_regs[self_reg]);
    }
    else if (instr->args.elems[Arg_SELF].data_type.is_signed) {
        DynamicStr_append_printf(output, "%d\n",
                instr->args.elems[Arg_SELF].value.imm_i32);
    }
    else if (instr->args.elems[Arg_SELF].data_type.is_signed) {
        DynamicStr_append_printf(output, "%u\n",
                instr->args.elems[Arg_SELF].value.imm_u32);
    }
    else
        assert(false);

}

static void gen_from_load_instr(struct DynamicStr *output,
        const struct IRInstr *instr, const struct IRRegLTList *vreg_lts,
        u32 cur_instr_idx) {

    u32 self_reg;
    u32 lhs_reg;

    assert(instr->args.size == 3);
    assert(instr->args.elems[Arg_SELF].type == IRInstrArg_REG);
    assert(instr->args.elems[Arg_LHS].type == IRInstrArg_REG);

    self_reg = virt_reg_to_cpu_reg(instr->args.elems[Arg_SELF].value.reg_name,
            vreg_lts, cur_instr_idx);

    lhs_reg = virt_reg_to_cpu_reg(instr->args.elems[Arg_LHS].value.reg_name,
            vreg_lts, cur_instr_idx);


    if (instr->args.elems[Arg_RHS].type == IRInstrArg_REG) {
        u32 rhs_reg = virt_reg_to_cpu_reg(
                instr->args.elems[Arg_RHS].value.reg_name, vreg_lts,
                cur_instr_idx);

        DynamicStr_append_printf(output, "%mov %s, [%s+%s]\n",
                cpu_regs[self_reg], cpu_regs[lhs_reg], cpu_regs[rhs_reg]);
    }
    else {
        DynamicStr_append_printf(output, "mov %s, [%s+%d]\n",
                cpu_regs[self_reg], cpu_regs[lhs_reg],
                instr->args.elems[Arg_RHS].value.imm_i32);
    }

}

static void gen_from_instr(struct DynamicStr *output,
        const struct IRInstr *instr,
        ATTRIBUTE((unused)) const struct TranslUnit *tu,
        const struct IRRegLTList *vreg_lts, u32 cur_instr_idx) {

    u32 i;

    if (instr->type == IRInstr_DIV) {
        gen_from_div_instr(output, instr, vreg_lts, cur_instr_idx);
    }
    /* important that this comes after DIV */
    else if (IRInstrType_is_bin_op(instr->type)) {
        gen_from_bin_instr(output, instr, vreg_lts, cur_instr_idx);
    }
    else if (IRInstrType_is_cond_branch(instr->type)) {
        gen_from_cond_jmp_instr(output, instr, vreg_lts, cur_instr_idx);
    }
    else if (IRInstrType_is_branch(instr->type)) {
        gen_from_uncond_jmp_instr(output, instr);
    }
    else if (instr->type == IRInstr_RET) {
        gen_from_ret_instr(output, instr, vreg_lts, cur_instr_idx);
    }
    else if (instr->type == IRInstr_ALLOCA) {
        gen_from_alloca_instr(output, instr, vreg_lts, cur_instr_idx);
    }
    else if (instr->type == IRInstr_STORE) {
        gen_from_store_instr(output, instr, vreg_lts, cur_instr_idx);
    }
    else if (instr->type == IRInstr_LOAD) {
        gen_from_load_instr(output, instr, vreg_lts, cur_instr_idx);
    }
    else {
        DynamicStr_append_printf(output, "%s ",
                X86_get_instr(instr->type, IRInstr_data_type(instr)));

        for (i = 0; i < instr->args.size; i++) {
            if (i > 0)
                DynamicStr_append(output, ", ");
            emit_instr_operand(output, &instr->args.elems[i], vreg_lts,
                    cur_instr_idx);
        }
        DynamicStr_append(output, "\n");
    }

}

static void gen_from_basic_block(struct DynamicStr *output,
        const struct IRBasicBlock *block, const struct TranslUnit *tu,
        const struct IRRegLTList *vreg_lts, u32 *n_instrs) {

    u32 i;

    DynamicStr_append_printf(output, ".%s:\n", block->label);

    for (i = 0; i < block->instrs.size; i++) {

        gen_from_instr(output, &block->instrs.elems[i], tu, vreg_lts,
                *n_instrs);

        ++*n_instrs;

    }

}

static void gen_from_func(struct DynamicStr *output,
        const struct IRFunc *func, const struct TranslUnit *tu) {

    u32 i;
    u32 n_instrs = 0;

    struct IRRegLTList vreg_lts = IRRegLTList_get_func_lts(func);

    init_cpu_reg_vals();

    DynamicStr_append_printf(output, "%s:\n", func->name);

    for (i = 0; i < func->blocks.size; i++) {

        gen_from_basic_block(output, &func->blocks.elems[i], tu, &vreg_lts,
                &n_instrs);

    }

    free_cpu_reg_vals();

    /* in case there was no explicit return */
    DynamicStr_append_printf(output, "sub esp, %d\nret\n", sp);
    sp = 0;

    while (vreg_lts.size > 0) {
        IRRegLTList_pop_back(&vreg_lts, IRRegLT_free);
    }
    IRRegLTList_free(&vreg_lts);

}

static void gen_from_module(struct DynamicStr *output,
        const struct IRModule *module, const struct TranslUnit *tu) {

    u32 i;

    for (i = 0; i < module->funcs.size; i++) {

        gen_from_func(output, &module->funcs.elems[i], tu);

    }

}

char* gen_x86_from_ir(const struct IRModule *module,
        const struct TranslUnit *tu) {

    struct DynamicStr output = DynamicStr_init();

    DynamicStr_append(&output, "[BITS 32]\n");

    DynamicStr_append(&output, "\nsection .text\n");

    DynamicStr_append(&output, "\nglobal main\n\n");

    gen_from_module(&output, module, tu);

    return output.str;

}
