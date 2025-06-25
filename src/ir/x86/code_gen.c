#include "code_gen.h"
#include "../../utils/dyn_str.h"
#include "../../macros.h"
#include "instrs.h"
#include <stdlib.h>
#include <assert.h>

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

/* if a virtual reg maps to a cpu reg, it's index must be one of these.
 * edx isn't used as a gp reg due to the generational fuck-up that is the div
 * instruction. */
u32 gp_regs[] = {
    CPUReg_AX, CPUReg_BX, CPUReg_CX,  CPUReg_SI, CPUReg_DI
};

/* breaks if there are more virtual registers than physical ones */
static u32 virt_reg_to_cpu_reg(const char *virt_reg) {

    u32 virt_reg_num = strtoul(virt_reg, NULL, 0);

    return gp_regs[virt_reg_num % m_arr_size(gp_regs)];

}

static void load_operand_to_reg(struct DynamicStr *output, u32 reg_idx,
        const struct IRInstrArg *operand) {

    if (operand->type == IRInstrArg_REG) {
        u32 operand_phys_reg = virt_reg_to_cpu_reg(operand->value.reg_name);
        assert(operand_phys_reg != reg_idx);

        DynamicStr_append_printf(output, "mov %s, %s\n",
                cpu_regs[reg_idx], cpu_regs[operand_phys_reg]);
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
        const struct IRInstrArg *arg) {

    if (arg->type == IRInstrArg_REG) {
        u32 operand_phys_reg = virt_reg_to_cpu_reg(arg->value.reg_name);

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
        const struct IRInstr *instr) {

    u32 self_reg;

    assert(instr->args.size == 3);
    assert(instr->args.elems[0].type == IRInstrArg_REG);

    self_reg = virt_reg_to_cpu_reg(instr->args.elems[0].value.reg_name);

    /* since x86 is a 2 operand architecture, but MCCIR is a 3 operand IL,
     * instructions will need to be converted from 3 to 2 operands. The
     * conversion looks like this:
     *    add r0 <- r1, r2
     * becomes:
     *    mov r0 <- r1
     *    add r1 <- r2
     */
    load_operand_to_reg(output, self_reg, &instr->args.elems[1]);

    DynamicStr_append_printf(output, "%s %s, ",
            X86_get_instr(instr->type, IRInstr_data_type(instr)),
            cpu_regs[self_reg]);

    emit_instr_operand(output, &instr->args.elems[2]);
    DynamicStr_append(output, "\n");

}

static void gen_from_div_instr(struct DynamicStr *output,
        const struct IRInstr *instr) {

    u32 self_reg;
    u32 rhs_reg;

    assert(instr->args.size == 3);
    assert(instr->args.elems[0].type == IRInstrArg_REG);

    /* get the rhs first, in case moving self into ax requires messing with
     * the stack, with would make all the stack offsets off by 4 bytes */
    if (instr->args.elems[2].type == IRInstrArg_REG) {
        rhs_reg = virt_reg_to_cpu_reg(instr->args.elems[2].value.reg_name);
    }
    else if (instr->args.elems[2].type == IRInstrArg_IMM32 &&
            instr->args.elems[2].data_type.is_signed) {
        rhs_reg = CPUReg_DX;
        DynamicStr_append_printf(output, "mov edx, %d\n",
                instr->args.elems[2].value.imm_i32);
    }
    else if (instr->args.elems[2].type == IRInstrArg_IMM32 &&
            instr->args.elems[2].data_type.is_signed) {
        rhs_reg = CPUReg_DX;
        DynamicStr_append_printf(output, "mov edx, %u\n",
                instr->args.elems[2].value.imm_u32);
    }
    else {
        assert(false);
    }

    self_reg = virt_reg_to_cpu_reg(instr->args.elems[0].value.reg_name);
    if (self_reg != CPUReg_AX) {
        DynamicStr_append(output, "push eax\n");
    }
    load_operand_to_reg(output, CPUReg_AX, &instr->args.elems[1]);

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
        const struct IRInstrArg *cmp_lhs, const struct IRInstrArg *cmp_rhs) {

    DynamicStr_append(output, "cmp ");
    emit_instr_operand(output, cmp_lhs);
    DynamicStr_append(output, ", ");
    emit_instr_operand(output, cmp_rhs);
    DynamicStr_append(output, "\n");

}

static void gen_from_cond_jmp_instr(struct DynamicStr *output,
        const struct IRInstr *instr) {

    assert(instr->args.size == 3);
    assert(instr->args.elems[2].type == IRInstrArg_STR);

    /* the conditional jump will get split into a seperate cmp and jmp
     * instruction. */

    gen_cmp_instr(output, &instr->args.elems[0], &instr->args.elems[1]);

    DynamicStr_append_printf(output, "%s .%s\n",
            X86_get_instr(instr->type, IRInstr_data_type(instr)),
            instr->args.elems[2].value.generic_str);

}

static void gen_from_uncond_jmp_instr(struct DynamicStr *output,
        const struct IRInstr *instr) {

    assert(instr->args.size == 1);
    assert(instr->args.elems[0].type == IRInstrArg_STR);

    DynamicStr_append_printf(output, "jmp .%s\n",
            instr->args.elems[0].value.generic_str);

}

static void gen_from_instr(struct DynamicStr *output,
        const struct IRInstr *instr,
        ATTRIBUTE((unused)) const struct TranslUnit *tu) {

    u32 i;

    if (instr->type == IRInstr_DIV) {
        gen_from_div_instr(output, instr);
    }
    /* important that this comes after DIV */
    else if (IRInstrType_is_bin_op(instr->type)) {
        gen_from_bin_instr(output, instr);
    }
    else if (IRInstrType_is_cond_branch(instr->type)) {
        gen_from_cond_jmp_instr(output, instr);
    }
    else if (IRInstrType_is_branch(instr->type)) {
        gen_from_uncond_jmp_instr(output, instr);
    }
    else {
        DynamicStr_append_printf(output, "%s ",
                X86_get_instr(instr->type, IRInstr_data_type(instr)));

        for (i = 0; i < instr->args.size; i++) {
            if (i > 0)
                DynamicStr_append(output, ", ");
            emit_instr_operand(output, &instr->args.elems[i]);
        }
        DynamicStr_append(output, "\n");
    }

}

static void gen_from_basic_block(struct DynamicStr *output,
        const struct IRBasicBlock *block, const struct TranslUnit *tu) {

    u32 i;

    DynamicStr_append_printf(output, ".%s:\n", block->label);

    for (i = 0; i < block->instrs.size; i++) {

        gen_from_instr(output, &block->instrs.elems[i], tu);

    }

}

static void gen_from_func(struct DynamicStr *output,
        const struct IRFunc *func, const struct TranslUnit *tu) {

    u32 i;

    DynamicStr_append_printf(output, "%s:\n", func->name);

    for (i = 0; i < func->blocks.size; i++) {

        gen_from_basic_block(output, &func->blocks.elems[i], tu);

    }

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
