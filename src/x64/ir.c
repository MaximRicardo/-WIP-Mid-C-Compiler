#include "ir.h"
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

unsigned n_gp_regs = 11;

/* is the register currently holding a value? */
bool gp_reg_used[] = {
    false,  /* ax */
    false,  /* bx */
    false,  /* cx */
    /* DX has been removed to make implementing mul and div easier */
    /*false,*/  /* dx */
    false,  /* r8 */
    false,  /* r9 */
    false,  /* r10 */
    false,  /* r11 */
    false,  /* r12 */
    false,  /* r13 */
    false,  /* r14 */
    false,  /* r15 */
};

struct GPReg {

    unsigned reg_idx;
    bool prev_val_was_leaked;

};

static struct GPReg GPReg_init(void) {

    struct GPReg reg;
    reg.reg_idx = UINT_MAX;
    reg.prev_val_was_leaked = false;
    return reg;

}

/* finds an unused register, returns UINT_MAX if one couldn't be found */
static unsigned unused_reg(void) {

    unsigned i;
    for (i = 0; i < n_gp_regs; i++) {
        if (!gp_reg_used[i])
            return i;
    }

    return UINT_MAX;

}

static void leak_reg_to_stack(struct InstrList *instrs, unsigned reg_idx) {

    struct Instruction instr = Instruction_init();
    assert(reg_idx < n_gp_regs);
    instr.lhs.type = InstrOperandType_REG_AX+reg_idx;
    instr.type = InstrType_PUSH;
    instr.instr_size = InstrSize_64;

    InstrList_push_back(instrs, instr);

}

static void unleak_reg(struct InstrList *instrs, unsigned reg_idx) {

    struct Instruction instr = Instruction_init();
    assert(reg_idx < n_gp_regs);
    instr.lhs.type = InstrOperandType_REG_AX+reg_idx;
    instr.type = InstrType_POP;
    instr.instr_size = InstrSize_64;

    InstrList_push_back(instrs, instr);

}

/* finds a free register to use. if there are no free registers left, a used
 * one is leaked onto the stack so it can be used. */
static struct GPReg alloc_reg(struct InstrList *instrs) {

    struct GPReg reg = GPReg_init();
    reg.reg_idx = unused_reg();
    if (reg.reg_idx == UINT_MAX) {
        /* IMPORTANT: verify that using rand isn't absolutely bonkers here */
        reg.reg_idx = rand() % n_gp_regs;
        leak_reg_to_stack(instrs, reg.reg_idx);
        reg.prev_val_was_leaked = true;
    }
    gp_reg_used[reg.reg_idx] = true;
    return reg;

}

static void free_reg(struct InstrList *instrs, struct GPReg reg) {

    if (reg.prev_val_was_leaked) {
        unleak_reg(instrs, reg.reg_idx);
    }
    else {
        gp_reg_used[reg.reg_idx] = false;
    }

    reg.reg_idx = UINT_MAX;

}

static enum InstrType expr_to_instr_t(const struct Expr *expr) {

    switch (expr->expr_type) {

    case ExprType_PLUS:
        return InstrType_ADD;

    case ExprType_MINUS:
        return InstrType_SUB;

    case ExprType_MUL:
        if (PrimitiveType_signed(Expr_type(expr)))
            return InstrType_IMUL;
        else
            return InstrType_MUL;

    case ExprType_DIV:
        if (PrimitiveType_signed(Expr_type(expr)))
            return InstrType_IDIV;
        else
            return InstrType_DIV;

    default:
        assert(false);

    }

}

struct InstrOperand InstrOperand_init(void) {

    struct InstrOperand operand;
    operand.type = InstrOperandType_INVALID;
    memset(&operand.value, 0, sizeof(operand.value));
    return operand;

}

struct InstrOperand InstrOperand_create(enum InstrOperandType type,
        union InstrOperandVal value) {

    struct InstrOperand operand;
    operand.type = type;
    operand.value = value;
    return operand;

}

struct InstrOperand InstrOperand_create_imm(enum InstrOperandType type,
        u32 imm) {

    union InstrOperandVal value;
    struct InstrOperand operand;

    value.imm = imm;
    operand = InstrOperand_create(type, value);

    return operand;

}

struct Instruction Instruction_init(void) {

    struct Instruction instr;
    instr.type = InstrType_INVALID;
    instr.instr_size = 0;
    instr.lhs = InstrOperand_init();
    instr.rhs = InstrOperand_init();
    return instr;

}

static enum InstrOperandType reg_idx_to_operand_t(unsigned idx) {

    return InstrOperandType_REG_AX+idx;

}

/* returns the register holding the result of the expression */
static struct GPReg get_expr_instructions(struct InstrList *instrs,
        const struct Expr *expr) {

    struct Instruction instr = Instruction_init();

    struct GPReg lhs_reg = GPReg_init(), rhs_reg = GPReg_init();

    if (expr->lhs)
        lhs_reg = get_expr_instructions(instrs, expr->lhs);
    else
        lhs_reg = alloc_reg(instrs);

    if (expr->rhs && expr->rhs->expr_type != ExprType_INT_LIT)
        rhs_reg = get_expr_instructions(instrs, expr->rhs);

    instr.instr_size = InstrSize_32;
    if (expr->expr_type == ExprType_INT_LIT) {
        instr.type = InstrType_MOV;
        instr.lhs = InstrOperand_create_imm(
                reg_idx_to_operand_t(lhs_reg.reg_idx), 0);
        instr.rhs = InstrOperand_create_imm(InstrOperandType_IMM_32,
                expr->int_value);
    }
    else {
        instr.type = expr_to_instr_t(expr);
        instr.lhs = InstrOperand_create_imm(
                reg_idx_to_operand_t(lhs_reg.reg_idx), 0);
        if (expr->rhs->expr_type != ExprType_INT_LIT)
            instr.rhs = InstrOperand_create_imm(
                    reg_idx_to_operand_t(rhs_reg.reg_idx), 0);
        else
            instr.rhs = InstrOperand_create_imm(InstrOperandType_IMM_32,
                    expr->rhs->int_value);
    }

    if (rhs_reg.reg_idx != UINT_MAX)
        free_reg(instrs, rhs_reg);

    InstrList_push_back(instrs, instr);

    return lhs_reg;

}

struct InstrList Instruction_get_instructions(const struct TUNode *tu) {

    struct InstrList instrs = InstrList_init();
    unsigned i;

    for (i = 0; i < tu->exprs.size; i++) {

        free_reg(&instrs, get_expr_instructions(&instrs, tu->exprs.elems[i]));

    }

    return instrs;

}

m_define_VectorImpl_funcs(InstrList, struct Instruction)
