#include "ir.h"
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

/* is the register currently holding a value? */
bool gp_reg_used[] = {
    false,  /* ax */
    false,  /* bx */
    false,  /* cx */
    false,  /* r8 */
    false,  /* r9 */
    false,  /* r10 */
    false,  /* r11 */
    false,  /* r12 */
    false,  /* r13 */
    false,  /* r14 */
    false,  /* r15 */
};

const unsigned n_gp_regs = sizeof(gp_reg_used)/sizeof(gp_reg_used[0]);

struct GPReg {

    unsigned reg_idx;
    bool prev_val_was_leaked;

};

static void get_block_instructions(struct InstrList *instrs,
        const struct BlockNode *block);

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

    case ExprType_MODULUS:
        if (PrimitiveType_signed(Expr_type(expr)))
            return InstrType_IMODULO;
        else
            return InstrType_MODULO;

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
    instr.offset = 0;
    instr.lhs = InstrOperand_init();
    instr.rhs = InstrOperand_init();
    instr.string = NULL;
    return instr;

}

void Instruction_free(struct Instruction instr) {

    m_free(instr.string);

}

static enum InstrOperandType reg_idx_to_operand_t(unsigned idx) {

    return InstrOperandType_REG_AX+idx;

}

/* load_reference is only used on identifier nodes, else it's ignored */
static struct GPReg get_expr_instructions(struct InstrList *instrs,
        const struct Expr *expr, bool load_reference) {

    struct Instruction instr = Instruction_init();

    struct GPReg lhs_reg = GPReg_init(), rhs_reg = GPReg_init();

    if (expr->lhs)
        lhs_reg = get_expr_instructions(instrs, expr->lhs,
                expr->expr_type == ExprType_EQUAL);
    else
        lhs_reg = alloc_reg(instrs);

    if (expr->rhs && expr->rhs->expr_type != ExprType_INT_LIT)
        rhs_reg = get_expr_instructions(instrs, expr->rhs, false);

    instr.instr_size = InstrSize_32;
    if (expr->expr_type == ExprType_INT_LIT) {
        instr.type = InstrType_MOV;
        instr.lhs = InstrOperand_create_imm(
                reg_idx_to_operand_t(lhs_reg.reg_idx), 0);
        instr.rhs = InstrOperand_create_imm(InstrOperandType_IMM_32,
                expr->int_value);
    }
    else if (expr->expr_type == ExprType_IDENT) {
        instr.type = load_reference ? InstrType_LEA : InstrType_MOV_F_LOC;
        instr.lhs = InstrOperand_create_imm(
                reg_idx_to_operand_t(lhs_reg.reg_idx), 0
                );
        instr.rhs = InstrOperand_create_imm(InstrOperandType_REG_BP, 0);
        instr.offset = expr->bp_offset;
    }
    else if (expr->expr_type == ExprType_EQUAL) {
        instr.type = InstrType_MOV_T_LOC;
        instr.lhs = InstrOperand_create_imm(
                reg_idx_to_operand_t(lhs_reg.reg_idx), 0
                );
        instr.rhs = InstrOperand_create_imm(InstrOperandType_REG_BP, 0);
        instr.offset = 0;
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

static void get_var_decl_instructions(struct InstrList *instrs,
    const struct VarDeclNode *var_decl) {

    unsigned i;
    for (i = 0; i < var_decl->decls.size; i++) {
        struct Instruction instr = Instruction_init();

        if (var_decl->decls.elems[i].value) {
            struct GPReg reg =
                get_expr_instructions(instrs, var_decl->decls.elems[i].value,
                        false);
            instr.type = InstrType_MOV_T_LOC;
            instr.instr_size = InstrSize_32;
            instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_BP, 0);
            instr.rhs = InstrOperand_create_imm(
                    reg_idx_to_operand_t(reg.reg_idx), 0);
            instr.offset = var_decl->decls.elems[i].bp_offset;
            free_reg(instrs, reg);
        }

        InstrList_push_back(instrs, instr);
    }

}

static void push_callee_saved_regs(struct InstrList *instrs) {

    struct Instruction push_instr = Instruction_init();
    push_instr.type = InstrType_PUSH;
    push_instr.instr_size = InstrSize_64;

    push_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_BX, 0);
    InstrList_push_back(instrs, push_instr);
    push_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_DI, 0);
    InstrList_push_back(instrs, push_instr);
    push_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_SI, 0);
    InstrList_push_back(instrs, push_instr);
    push_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_R12, 0);
    InstrList_push_back(instrs, push_instr);
    push_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_R13, 0);
    InstrList_push_back(instrs, push_instr);
    push_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_R14, 0);
    InstrList_push_back(instrs, push_instr);
    push_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_R15, 0);
    InstrList_push_back(instrs, push_instr);

}

static void pop_callee_saved_regs(struct InstrList *instrs) {

    struct Instruction pop_instr = Instruction_init();
    pop_instr.type = InstrType_POP;
    pop_instr.instr_size = InstrSize_64;

    pop_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_R15, 0);
    InstrList_push_back(instrs, pop_instr);
    pop_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_R14, 0);
    InstrList_push_back(instrs, pop_instr);
    pop_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_R13, 0);
    InstrList_push_back(instrs, pop_instr);
    pop_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_R12, 0);
    InstrList_push_back(instrs, pop_instr);
    pop_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_SI, 0);
    InstrList_push_back(instrs, pop_instr);
    pop_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_DI, 0);
    InstrList_push_back(instrs, pop_instr);
    pop_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_BX, 0);
    InstrList_push_back(instrs, pop_instr);

}

static void create_stack_frame(struct InstrList *instrs, u32 var_bytes) {

    struct Instruction push_instr = Instruction_init();
    struct Instruction mov_instr = Instruction_init();
    struct Instruction alloc_instr = Instruction_init();

    push_instr.type = InstrType_PUSH;
    push_instr.instr_size = InstrSize_64;
    push_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_BP, 0);

    mov_instr.type = InstrType_MOV;
    mov_instr.instr_size = InstrSize_64;
    mov_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_BP, 0);
    mov_instr.rhs = InstrOperand_create_imm(InstrOperandType_REG_SP, 0);

    alloc_instr.type = InstrType_SUB;
    alloc_instr.instr_size = InstrSize_64;
    alloc_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_SP, 0);
    alloc_instr.rhs =
        InstrOperand_create_imm(InstrOperandType_IMM_32, var_bytes);

    InstrList_push_back(instrs, push_instr);
    InstrList_push_back(instrs, mov_instr);
    InstrList_push_back(instrs, alloc_instr);

}

static void destroy_stack_frame(struct InstrList *instrs) {

    struct Instruction mov_instr = Instruction_init();
    struct Instruction push_instr = Instruction_init();

    mov_instr.type = InstrType_MOV;
    mov_instr.instr_size = InstrSize_64;
    mov_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_SP, 0);
    mov_instr.rhs = InstrOperand_create_imm(InstrOperandType_REG_BP, 0);

    push_instr.type = InstrType_POP;
    push_instr.instr_size = InstrSize_64;
    push_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_BP, 0);

    InstrList_push_back(instrs, mov_instr);
    InstrList_push_back(instrs, push_instr);

}

static void get_func_decl_instructions(struct InstrList *instrs,
        const struct FuncDeclNode *func) {

    struct Instruction label_instr = Instruction_init();
    struct Instruction ret_instr = Instruction_init();

    if (!func->body)
        return;

    label_instr.type = InstrType_LABEL;
    label_instr.string =
        safe_malloc((strlen(func->name)+1)*sizeof(*label_instr.string));
    strcpy(label_instr.string, func->name);
    InstrList_push_back(instrs, label_instr);

    push_callee_saved_regs(instrs);
    create_stack_frame(instrs, func->body->var_bytes);

    get_block_instructions(instrs, func->body);

    destroy_stack_frame(instrs);
    pop_callee_saved_regs(instrs);
    ret_instr.type = InstrType_RET;
    InstrList_push_back(instrs, ret_instr);

}

static void get_block_instructions(struct InstrList *instrs,
        const struct BlockNode *block) {

    unsigned i;

    for (i = 0; i < block->nodes.size; i++) {

        void *node_struct = block->nodes.elems[i].node_struct;

        if (block->nodes.elems[i].type == ASTType_EXPR)
            free_reg(instrs, get_expr_instructions(instrs,
                        ((const struct ExprNode*)node_struct)->expr, false));
        else if (block->nodes.elems[i].type == ASTType_VAR_DECL)
            get_var_decl_instructions(instrs,
                    (const struct VarDeclNode*)node_struct);
        else if (block->nodes.elems[i].type == ASTType_FUNC)
            get_func_decl_instructions(instrs,
                    (const struct FuncDeclNode*)node_struct);
        else if (block->nodes.elems[i].type == ASTType_BLOCK) {
            const struct BlockNode *new_block =
                block->nodes.elems[i].node_struct;
            create_stack_frame(instrs, new_block->var_bytes);
            get_block_instructions(instrs, new_block);
            destroy_stack_frame(instrs);
        }
        else if (block->nodes.elems[i].type == ASTType_DEBUG_RAX) {
            struct Instruction debug_instr = Instruction_init();
            debug_instr.type = InstrType_DEBUG_RAX;
            InstrList_push_back(instrs, debug_instr);
        }

    }

}

struct InstrList IR_get_instructions(const struct BlockNode *ast) {

    struct InstrList instrs = InstrList_init();

    get_block_instructions(&instrs, ast);

    return instrs;

}

m_define_VectorImpl_funcs(InstrList, struct Instruction)
