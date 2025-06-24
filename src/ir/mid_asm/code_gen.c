#include "code_gen.h"
#include "../../utils/dyn_str.h"
#include "../../macros.h"
#include "instrs.h"
#include <stdlib.h>
#include <assert.h>

enum CPURegs {

    CPUReg_R0,
    CPUReg_R1,
    CPUReg_R2,
    CPUReg_R3,
    CPUReg_R4,
    CPUReg_R5,
    CPUReg_R6,
    CPUReg_R7,
    CPUReg_R8,
    CPUReg_R9,
    CPUReg_R10,
    CPUReg_R11,
    CPUReg_R12,
    CPUReg_R13,
    CPUReg_R14,
    CPUReg_R15

};

/* must be in the same order as enum CPURegs. */
char *cpu_regs[] = {

    "r0",
    "r1",
    "r2",
    "r3",
    "r4",
    "r5",
    "r6",
    "r7",
    "r8",
    "r9",
    "r10",
    "r11",
    "r12",
    "r13",
    "r14",
    "r15",

};

/* if a virtual reg maps to a cpu reg, it's index must be one of these.
 * edx isn't used as a gp reg due to the generational fuck-ups that are the
 * mul and div instructions. */
u32 gp_regs[] = {
    CPUReg_R1, CPUReg_R2, CPUReg_R3, CPUReg_R4, CPUReg_R5, CPUReg_R6,
    CPUReg_R7, CPUReg_R8, CPUReg_R9, CPUReg_R10, CPUReg_R11, CPUReg_R12,
    CPUReg_R13, CPUReg_R14, CPUReg_R15,
};

/* breaks if there are more virtual registers than physical ones */
static u32 virt_reg_to_cpu_reg(const char *virt_reg) {

    u32 virt_reg_num = strtoul(virt_reg, NULL, 0);

    return gp_regs[virt_reg_num % m_arr_size(gp_regs)];

}

/*
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

*/

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
    else {
        assert(false);
    }

}

static void gen_from_instr(struct DynamicStr *output,
        const struct IRInstr *instr,
        ATTRIBUTE((unused)) const struct TranslUnit *tu) {

    u32 self_reg;

    assert(instr->args.size == 3);
    assert(instr->args.elems[0].type == IRInstrArg_REG);

    self_reg = virt_reg_to_cpu_reg(instr->args.elems[0].value.reg_name);

    DynamicStr_append_printf(output, "%s %s, ",
            MidAsm_get_instr(instr->type, IRInstr_data_type(instr)),
            cpu_regs[self_reg]);

    emit_instr_operand(output, &instr->args.elems[1]);
    DynamicStr_append(output, ", ");
    emit_instr_operand(output, &instr->args.elems[2]);
    DynamicStr_append(output, "\n");

}

static void gen_from_basic_block(struct DynamicStr *output,
        const struct IRBasicBlock *block, const struct TranslUnit *tu) {

    u32 i;

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

char* gen_Mid_Asm_from_ir(const struct IRModule *module,
        const struct TranslUnit *tu) {

    struct DynamicStr output = DynamicStr_init();

    gen_from_module(&output, module, tu);

    return output.str;

}
