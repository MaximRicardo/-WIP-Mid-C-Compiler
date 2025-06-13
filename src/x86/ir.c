#include "ir.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* the length of the labels the compiler generates for stuff like loops and
 * if statements, not the labels used for function names and stuff.
 * i don't think anyone's ever gonna have a TU so long that they're gonna need
 * label names long than this */
#define m_comp_label_name_capacity 1024

unsigned long label_counter = 0;

unsigned next_reg_to_leak = 0;

/* is the register currently holding a value? */
bool gp_reg_used[] = {
    false,  /* ax */
    false,  /* bx */
    false,  /* cx */
};

const unsigned n_gp_regs = sizeof(gp_reg_used)/sizeof(gp_reg_used[0]);

struct GPReg {

    unsigned reg_idx;
    enum InstrSize reg_size;
    bool prev_val_was_leaked;

};

static struct GPReg GPReg_init(void) {

    struct GPReg reg;
    reg.reg_idx = UINT_MAX;
    reg.reg_size = InstrSize_32;
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

static u32 round_up(u32 num, u32 multiple) {

    u32 remainder;

    if (multiple == 0)
        return num;

    remainder = num % multiple;
    if (remainder == 0)
        return num;

    return num+multiple-remainder;

}

static void get_block_instructions(struct InstrList *instrs,
        const struct BlockNode *block);

static void leak_reg_to_stack(struct InstrList *instrs, unsigned reg_idx) {

    struct Instruction instr = Instruction_init();
    assert(reg_idx < n_gp_regs);
    instr.lhs.type = InstrOperandType_REG_AX+reg_idx;
    instr.type = InstrType_PUSH;
    instr.instr_size = InstrSize_32;

    InstrList_push_back(instrs, instr);

}

static void unleak_reg(struct InstrList *instrs, unsigned reg_idx) {

    struct Instruction instr = Instruction_init();
    assert(reg_idx < n_gp_regs);
    instr.lhs.type = InstrOperandType_REG_AX+reg_idx;
    instr.type = InstrType_POP;
    instr.instr_size = InstrSize_32;

    InstrList_push_back(instrs, instr);

}

/* finds a free register to use. if there are no free registers left, a used
 * one is leaked onto the stack so it can be used. */
static struct GPReg alloc_reg(struct InstrList *instrs) {

    struct GPReg reg = GPReg_init();
    reg.reg_idx = unused_reg();
    if (reg.reg_idx == UINT_MAX) {
        /* IMPORTANT: verify that using rand isn't absolutely bonkers here */
        reg.reg_idx = next_reg_to_leak++;
        leak_reg_to_stack(instrs, reg.reg_idx);
        reg.prev_val_was_leaked = true;
        next_reg_to_leak %= n_gp_regs;
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

static unsigned operand_t_to_reg_idx(enum InstrOperandType type) {

    return type-InstrOperandType_REGISTERS_START-1;

}

static bool operand_t_is_reg(enum InstrOperandType type) {

    return type > InstrOperandType_REGISTERS_START &&
        type < InstrOperandType_REGISTERS_END;

}

static enum InstrType expr_to_instr_t(const struct Expr *expr) {

    switch (expr->expr_type) {

    case ExprType_PLUS:
        return InstrType_ADD;

    case ExprType_MINUS:
        return InstrType_SUB;

    case ExprType_MUL:
        if (PrimitiveType_signed(expr->prim_type,
                    expr->lvls_of_indir))
            return InstrType_IMUL;
        else
            return InstrType_MUL;

    case ExprType_DIV:
        if (PrimitiveType_signed(expr->prim_type,
                    expr->lvls_of_indir))
            return InstrType_IDIV;
        else
            return InstrType_DIV;

    case ExprType_MODULUS:
        if (PrimitiveType_signed(expr->prim_type,
                    expr->lvls_of_indir))
            return InstrType_IMODULO;
        else
            return InstrType_MODULO;

    case ExprType_BITWISE_NOT:
        return InstrType_BITWISE_NOT;

    case ExprType_NEGATIVE:
        return InstrType_NEGATIVE;

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

unsigned InstrSize_to_bytes(enum InstrSize size) {
    /* 2^size */
    return 1 << size;
}

enum InstrSize InstrSize_bytes_to(unsigned bytes) {

    if (bytes == 1)
        return InstrSize_8;
    else if (bytes == 2)
        return InstrSize_16;
    else if (bytes == 4)
        return InstrSize_32;
    else
        assert(false);

}

struct Instruction Instruction_init(void) {

    struct Instruction instr;
    instr.type = InstrType_INVALID;
    instr.is_unary = false;
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

    return InstrOperandType_REGISTERS_START+idx+1;

}

static void push_used_caller_saved_regs(struct InstrList *instrs) {

    struct Instruction push_instr = Instruction_init();
    push_instr.type = InstrType_PUSH;
    push_instr.instr_size = InstrSize_32;

    if (gp_reg_used[operand_t_to_reg_idx(InstrOperandType_REG_CX)]) {
        push_instr.lhs =
            InstrOperand_create_imm(InstrOperandType_REG_CX, 0);
        InstrList_push_back(instrs, push_instr);
    }

    /*
    if (gp_reg_used[operand_t_to_reg_idx(InstrOperandType_REG_DX)]) {
        push_instr.lhs =
            InstrOperand_create_imm(InstrOperandType_REG_DX, 0);
        InstrList_push_back(instrs, push_instr);
    }*/

}

static void pop_used_caller_saved_regs(struct InstrList *instrs) {

    struct Instruction pop_instr = Instruction_init();
    pop_instr.type = InstrType_POP;
    pop_instr.instr_size = InstrSize_32;

    /*
    if (gp_reg_used[operand_t_to_reg_idx(InstrOperandType_REG_DX)]) {
        pop_instr.lhs =
            InstrOperand_create_imm(InstrOperandType_REG_DX, 0);
        InstrList_push_back(instrs, pop_instr);
    }*/

    if (gp_reg_used[operand_t_to_reg_idx(InstrOperandType_REG_CX)]) {
        pop_instr.lhs =
            InstrOperand_create_imm(InstrOperandType_REG_CX, 0);
        InstrList_push_back(instrs, pop_instr);
    }

}

static void instr_reg_and_reg(struct InstrList *instrs, enum InstrType type,
        enum InstrSize size, enum InstrOperandType lhs_reg,
        enum InstrOperandType rhs_reg, i32 offset) {

    struct Instruction instr = Instruction_init();

    instr.type = type;
    instr.instr_size = size;
    instr.lhs = InstrOperand_create_imm(lhs_reg, 0);
    instr.rhs = InstrOperand_create_imm(rhs_reg, 0);
    instr.offset = offset;

    InstrList_push_back(instrs, instr);

}

static void instr_reg_and_imm32(struct InstrList *instrs, enum InstrType type,
        enum InstrSize size, enum InstrOperandType reg, u32 imm, i32 offset) {

    struct Instruction instr = Instruction_init();

    instr.type = type;
    instr.instr_size = size;
    instr.lhs = InstrOperand_create_imm(reg, 0);
    instr.rhs = InstrOperand_create_imm(InstrOperandType_IMM_32, imm);
    instr.offset = offset;

    InstrList_push_back(instrs, instr);

}

static void instr_reg(struct InstrList *instrs, enum InstrType type,
        enum InstrSize size, enum InstrOperandType reg, i32 offset) {

    struct Instruction instr = Instruction_init();

    instr.type = type;
    instr.instr_size = size;
    instr.lhs = InstrOperand_create_imm(reg, 0);
    instr.offset = offset;

    InstrList_push_back(instrs, instr);

}

/*
static void instr_imm32(struct InstrList *instrs, enum InstrType type,
        enum InstrSize size, u32 imm) {

    struct Instruction instr = Instruction_init();

    instr.type = type;
    instr.instr_size = size;
    instr.lhs = InstrOperand_create_imm(InstrOperandType_IMM_32, imm);

    InstrList_push_back(instrs, instr);

}*/

static void instr_string(struct InstrList *instrs, enum InstrType type,
        char *str) {

    struct Instruction instr = Instruction_init();

    instr.type = type;
    instr.string = str;

    InstrList_push_back(instrs, instr);

}

static void instr_only_type(struct InstrList *instrs, enum InstrType type) {

    struct Instruction instr = Instruction_init();

    instr.type = type;

    InstrList_push_back(instrs, instr);

}

/* returns the log2 of bytes. only works for 1, 2, and 4 bytes. i can't get
 * log2 and friends to work for some reason, my best guess is that c89 doesn't
 * support them? */
static unsigned bytes_log2(unsigned bytes) {

    switch (bytes) {

    case 1:
        return 0;

    case 2:
        return 1;

    case 4:
        return 2;

    default:
        assert(false);

    }

}

static struct GPReg get_expr_instructions(struct InstrList *instrs,
        const struct Expr *expr, bool load_reference);

static struct GPReg get_func_call_expr_instructions(struct InstrList *instrs,
        const struct Expr *expr) {

    u32 i;
    u32 args_stack_space = 0;
    u32 next_arg_offset = 0;

    struct GPReg ret_reg = alloc_reg(instrs);
    ret_reg.reg_size = InstrSize_32;

    push_used_caller_saved_regs(instrs);

    /* the return value always goes in ax, so this is to keep the old value of
     * rax if it is not to be overwritten, and also to make sure the return
     * value goes in the correct register */
    if (reg_idx_to_operand_t(ret_reg.reg_idx) != InstrOperandType_REG_AX)
        instr_reg_and_reg(instrs, InstrType_MOV, InstrSize_32,
                InstrOperandType_REG_AX, reg_idx_to_operand_t(ret_reg.reg_idx),
                0);

    /* make space for every argument on the stack. this method is slightly more
     * efficient and slightly easier to implement */
    {
        for (i = 0; i < expr->args.size; i++) {
            unsigned arg_size = PrimitiveType_size(
                    expr->args.elems[i]->prim_type,
                    expr->args.elems[i]->lvls_of_indir);
            /* alignment */
            args_stack_space = round_up(args_stack_space, arg_size);
            args_stack_space += arg_size;
        }

        instr_reg_and_imm32(instrs, InstrType_SUB, InstrSize_32,
                InstrOperandType_REG_SP, args_stack_space, 0);
    }

    /* every argument gets loaded into the stack from bottom to top */
    for (i = 0; i < expr->args.size; i++) {
        struct GPReg arg_reg = get_expr_instructions(instrs,
                expr->args.elems[i], false);
        unsigned reg_size_bytes = InstrSize_to_bytes(arg_reg.reg_size);

        next_arg_offset = round_up(next_arg_offset, reg_size_bytes);

        instr_reg_and_reg(instrs, InstrType_MOV_T_LOC, arg_reg.reg_size,
                InstrOperandType_REG_SP, reg_idx_to_operand_t(arg_reg.reg_idx),
                next_arg_offset);

        next_arg_offset += reg_size_bytes;

        free_reg(instrs, arg_reg);
    }

    /* everything's prepared now */
    instr_string(instrs, InstrType_CALL, Expr_src(expr));

    /* clean up the stack and bring back the caller saved regs */
    instr_reg_and_imm32(instrs, InstrType_ADD, InstrSize_32,
            InstrOperandType_REG_SP, args_stack_space, 0);
    pop_used_caller_saved_regs(instrs);

    if (reg_idx_to_operand_t(ret_reg.reg_idx) != InstrOperandType_REG_AX) {
        instr_reg_and_reg(instrs, InstrType_XCHG, InstrSize_32,
                InstrOperandType_REG_AX, reg_idx_to_operand_t(ret_reg.reg_idx),
                0);
    }
    return ret_reg;

}

/* load_reference is only used on identifier nodes and dereference nodes, else
 * it's ignored */
static struct GPReg get_expr_instructions(struct InstrList *instrs,
        const struct Expr *expr, bool load_reference) {

    struct GPReg lhs_reg = GPReg_init(), rhs_reg = GPReg_init();
    enum InstrSize instr_size = InstrSize_32;

    if (expr->expr_type == ExprType_FUNC_CALL) {
        return get_func_call_expr_instructions(instrs, expr);
    }

    if (expr->lhs)
        lhs_reg = get_expr_instructions(instrs, expr->lhs,
                expr->expr_type == ExprType_EQUAL ||
                expr->expr_type == ExprType_REFERENCE ||
                expr->expr_type == ExprType_L_ARR_SUBSCR ||
                expr->lhs->is_array);
    else
        lhs_reg = alloc_reg(instrs);

    if (expr->rhs && expr->rhs->expr_type != ExprType_INT_LIT)
        rhs_reg = get_expr_instructions(instrs, expr->rhs, false);

    if (expr->expr_type == ExprType_INT_LIT) {
        instr_reg_and_imm32(instrs, InstrType_MOV, instr_size,
                reg_idx_to_operand_t(lhs_reg.reg_idx), expr->int_value, 0);
    }
    else if (expr->expr_type == ExprType_IDENT) {
        enum InstrType type = load_reference ? InstrType_LEA :
            InstrType_MOV_F_LOC;
        unsigned var_size = PrimitiveType_size(expr->non_prom_prim_type,
                expr->lvls_of_indir+load_reference);
        instr_reg_and_reg(instrs, type, InstrSize_bytes_to(var_size),
                reg_idx_to_operand_t(lhs_reg.reg_idx), InstrOperandType_REG_BP,
                expr->bp_offset);
        if (var_size < 4) {
            instr_reg_and_imm32(instrs, InstrType_AND, instr_size,
                    reg_idx_to_operand_t(lhs_reg.reg_idx), (1<<var_size*8)-1,
                    0);
        }
    }
    else if (expr->expr_type == ExprType_EQUAL) {
        enum InstrSize assignment_size = InstrSize_bytes_to(
                PrimitiveType_size(expr->lhs_type,
                    expr->lhs_lvls_of_indir));

        if (expr->rhs->expr_type != ExprType_INT_LIT)
            instr_reg_and_reg(instrs, InstrType_MOV_T_LOC, assignment_size,
                    reg_idx_to_operand_t(lhs_reg.reg_idx),
                    reg_idx_to_operand_t(rhs_reg.reg_idx), 0);
        else
            instr_reg_and_imm32(instrs, InstrType_MOV_T_LOC, assignment_size,
                    reg_idx_to_operand_t(lhs_reg.reg_idx),
                    expr->rhs->int_value, 0);
    }
    else if (expr->expr_type == ExprType_POSITIVE) {

    }
    else if (expr->expr_type == ExprType_REFERENCE) {

    }
    else if (expr->expr_type == ExprType_DEREFERENCE && !load_reference) {
        unsigned deref_ptr_size = PrimitiveType_size(expr->lhs_og_type,
                        expr->lhs_lvls_of_indir-1);

        instr_reg_and_reg(instrs, InstrType_MOV_F_LOC,
                InstrSize_bytes_to(deref_ptr_size),
                reg_idx_to_operand_t(lhs_reg.reg_idx),
                reg_idx_to_operand_t(lhs_reg.reg_idx), 0);

        if (deref_ptr_size < 4) {
            instr_reg_and_imm32(instrs, InstrType_AND, InstrSize_32,
                    reg_idx_to_operand_t(lhs_reg.reg_idx),
                    (1<<deref_ptr_size*8)-1, 0);
        }
    }
    else if (expr->expr_type == ExprType_DEREFERENCE) {
        /* if load_reference is true then it cancels out the dereference */
    }
    else if (expr->expr_type == ExprType_L_ARR_SUBSCR) {
        unsigned deref_ptr_size = PrimitiveType_size(expr->lhs_og_type,
                        expr->lhs_lvls_of_indir-1);

        if (expr->rhs->expr_type != ExprType_INT_LIT) {
            instr_reg_and_imm32(instrs, InstrType_SHL, InstrSize_32,
                    reg_idx_to_operand_t(rhs_reg.reg_idx),
                    (1<<deref_ptr_size)-1, 0);

            instr_reg_and_reg(instrs, InstrType_ADD, InstrSize_32,
                    reg_idx_to_operand_t(lhs_reg.reg_idx),
                    reg_idx_to_operand_t(rhs_reg.reg_idx), 0);
        }
        else
            instr_reg_and_imm32(instrs, InstrType_ADD, InstrSize_32,
                    reg_idx_to_operand_t(lhs_reg.reg_idx),
                    expr->rhs->int_value*deref_ptr_size, 0);

        if (!load_reference) {
            instr_reg_and_reg(instrs, InstrType_MOV_F_LOC,
                    InstrSize_bytes_to(deref_ptr_size),
                    reg_idx_to_operand_t(lhs_reg.reg_idx),
                    reg_idx_to_operand_t(lhs_reg.reg_idx), 0);
        }
    }
    else {
        struct Instruction instr = Instruction_init();
        bool is_ptr_int_operation =
            expr->rhs && expr->lhs_lvls_of_indir > 0 &&
                expr->rhs_lvls_of_indir == 0;
        bool is_ptr_ptr_operation =
            expr->rhs && expr->lhs_lvls_of_indir > 0 &&
                expr->rhs_lvls_of_indir > 0;

        instr.type = expr_to_instr_t(expr);
        instr.instr_size = instr_size;
        instr.lhs = InstrOperand_create_imm(
                reg_idx_to_operand_t(lhs_reg.reg_idx), 0);
        if (!expr->rhs) {
            instr.is_unary = true;
        }
        else if (expr->rhs->expr_type != ExprType_INT_LIT)
            instr.rhs = InstrOperand_create_imm(
                    reg_idx_to_operand_t(rhs_reg.reg_idx), 0);
        else
            instr.rhs = InstrOperand_create_imm(InstrOperandType_IMM_32,
                    expr->rhs->int_value);

        if (is_ptr_int_operation && operand_t_is_reg(instr.rhs.type)) {
            instr_reg_and_imm32(instrs, InstrType_SHL, instr.instr_size,
                    instr.rhs.type,
                    bytes_log2(PrimitiveType_size(expr->lhs_og_type,
                            expr->lhs_lvls_of_indir-1)), 0);
        }
        else if (is_ptr_int_operation) {
            instr.rhs.value.imm <<=
                bytes_log2(PrimitiveType_size(expr->lhs_og_type,
                            expr->lhs_lvls_of_indir-1));
        }

        InstrList_push_back(instrs, instr);

        if (is_ptr_ptr_operation) {
            instr_reg_and_imm32(instrs, InstrType_SHR, instr.instr_size,
                    instr.lhs.type,
                    bytes_log2(PrimitiveType_size(expr->lhs_og_type,
                            expr->lhs_lvls_of_indir-1)), 0);
        }
    }

    if (rhs_reg.reg_idx != UINT_MAX)
        free_reg(instrs, rhs_reg);

    {
        struct GPReg ret_reg = lhs_reg;
        ret_reg.reg_size = instr_size;
        return ret_reg;
    }

}

static void get_var_decl_instructions(struct InstrList *instrs,
    const struct VarDeclNode *var_decl) {

    unsigned i;
    for (i = 0; i < var_decl->decls.size; i++) {
        if (var_decl->decls.elems[i].value) {
            enum InstrSize instr_size = InstrSize_bytes_to(PrimitiveType_size(
                        var_decl->type, var_decl->decls.elems[i].lvls_of_indir
                        ));

            struct GPReg reg =
                get_expr_instructions(instrs, var_decl->decls.elems[i].value,
                        false);
            instr_reg_and_reg(instrs, InstrType_MOV_T_LOC, instr_size,
                    InstrOperandType_REG_BP, reg_idx_to_operand_t(reg.reg_idx),
                    var_decl->decls.elems[i].bp_offset);

            free_reg(instrs, reg);
        }
    }

}

static void push_callee_saved_regs(struct InstrList *instrs) {

    struct Instruction push_instr = Instruction_init();
    push_instr.type = InstrType_PUSH;
    push_instr.instr_size = InstrSize_32;

    push_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_BX, 0);
    InstrList_push_back(instrs, push_instr);
    push_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_SI, 0);
    InstrList_push_back(instrs, push_instr);
    push_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_DI, 0);
    InstrList_push_back(instrs, push_instr);

}

static void pop_callee_saved_regs(struct InstrList *instrs) {

    struct Instruction pop_instr = Instruction_init();
    pop_instr.type = InstrType_POP;
    pop_instr.instr_size = InstrSize_32;

    pop_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_DI, 0);
    InstrList_push_back(instrs, pop_instr);
    pop_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_SI, 0);
    InstrList_push_back(instrs, pop_instr);
    pop_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_BX, 0);
    InstrList_push_back(instrs, pop_instr);

}

static void create_stack_frame(struct InstrList *instrs, u32 var_bytes) {

    struct Instruction push_instr = Instruction_init();
    struct Instruction mov_instr = Instruction_init();
    struct Instruction alloc_instr = Instruction_init();

    push_instr.type = InstrType_PUSH;
    push_instr.instr_size = InstrSize_32;
    push_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_BP, 0);

    mov_instr.type = InstrType_MOV;
    mov_instr.instr_size = InstrSize_32;
    mov_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_BP, 0);
    mov_instr.rhs = InstrOperand_create_imm(InstrOperandType_REG_SP, 0);

    alloc_instr.type = InstrType_SUB;
    alloc_instr.instr_size = InstrSize_32;
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
    mov_instr.instr_size = InstrSize_32;
    mov_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_SP, 0);
    mov_instr.rhs = InstrOperand_create_imm(InstrOperandType_REG_BP, 0);

    push_instr.type = InstrType_POP;
    push_instr.instr_size = InstrSize_32;
    push_instr.lhs = InstrOperand_create_imm(InstrOperandType_REG_BP, 0);

    InstrList_push_back(instrs, mov_instr);
    InstrList_push_back(instrs, push_instr);

}

static void get_func_decl_instructions(struct InstrList *instrs,
        const struct FuncDeclNode *func) {

    char *label = NULL;

    if (!func->body) {
        char *copy = safe_malloc((strlen(func->name)+1)*sizeof(*copy));
        strcpy(copy, func->name);
        instr_string(instrs, InstrType_EXTERN, copy);
        return;
    }

    label = safe_malloc((strlen(func->name)+1)*sizeof(*label));
    strcpy(label, func->name);
    instr_string(instrs, InstrType_LABEL, label);

    push_callee_saved_regs(instrs);
    create_stack_frame(instrs, func->body->var_bytes);

    get_block_instructions(instrs, func->body);

    destroy_stack_frame(instrs);
    pop_callee_saved_regs(instrs);
    instr_only_type(instrs, InstrType_RET);

}

static void get_ret_stmt_instructions(struct InstrList *instrs,
        const struct RetNode *ret_node) {

    u32 i;

    if (ret_node->value) {
        struct GPReg reg =
            get_expr_instructions(instrs, ret_node->value, false);
        assert(reg.reg_idx == 0);
        free_reg(instrs, reg);

        if (PrimitiveType_size(ret_node->type, ret_node->lvls_of_indir) < 4) {
            unsigned type_size = PrimitiveType_size(ret_node->type,
                    ret_node->lvls_of_indir);
            instr_reg_and_imm32(instrs, InstrType_AND, InstrSize_32,
                    InstrOperandType_REG_AX,
                    (1<<type_size*8)-1, 0);
        }
    }

    /* destroy all the stack frames to get to the return address.
     * this is a kinda ugly way of doing this but it's easy to implement so
     * i'ma do it this way */
    for (i = 0; i < ret_node->n_stack_frames_deep; i++) {
        instr_reg_and_reg(instrs, InstrType_MOV, InstrSize_32,
                InstrOperandType_REG_SP, InstrOperandType_REG_BP, 0);
        instr_reg(instrs, InstrType_POP, InstrSize_32, InstrOperandType_REG_BP,
                0);
    }

    pop_callee_saved_regs(instrs);

    instr_only_type(instrs, InstrType_RET);

}

static void get_if_stmt_instructions(struct InstrList *instrs,
        struct IfNode *if_node) {

    struct GPReg expr_reg;
    /* each instruction assumes it can free it's associated string, meaning
     * each instruction will need to be given a different one */
    char *if_end_label[2] = {NULL, NULL};
    char *else_end_label[2] = {NULL, NULL};
    u32 i;

    if (!if_node->body)
        return;

    for (i = 0; i < sizeof(if_end_label)/sizeof(if_end_label[0]); i++) {
        if_end_label[i] =
            safe_malloc(m_comp_label_name_capacity*sizeof(*if_end_label[i]));
        sprintf(if_end_label[i], "_L%lu$", label_counter);
        if (if_node->else_body) {
            else_end_label[i] = safe_malloc(
                    m_comp_label_name_capacity*sizeof(*else_end_label[i]));
            sprintf(else_end_label[i], "_L%lu$", label_counter+1);
        }
    }
    label_counter += 1+(if_node->else_body!=NULL);

    expr_reg = get_expr_instructions(instrs, if_node->expr, false);

    instr_reg_and_imm32(instrs, InstrType_CMP, expr_reg.reg_size,
            reg_idx_to_operand_t(expr_reg.reg_idx), 0, 0);
    instr_string(instrs, InstrType_JE, if_end_label[0]);

    free_reg(instrs, expr_reg);

    if (if_node->body_in_block)
        create_stack_frame(instrs, if_node->body->var_bytes);
    get_block_instructions(instrs, if_node->body);
    if (if_node->body_in_block)
        destroy_stack_frame(instrs);

    if (if_node->else_body)
        instr_string(instrs, InstrType_JMP, else_end_label[0]);

    instr_string(instrs, InstrType_LABEL, if_end_label[1]);

    if (if_node->else_body) {
        if (if_node->else_body_in_block)
            create_stack_frame(instrs, if_node->else_body->var_bytes);
        get_block_instructions(instrs, if_node->else_body);
        if (if_node->else_body_in_block)
            destroy_stack_frame(instrs);

        instr_string(instrs, InstrType_LABEL, else_end_label[1]);
    }

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
            get_var_decl_instructions(instrs, node_struct);
        else if (block->nodes.elems[i].type == ASTType_FUNC)
            get_func_decl_instructions(instrs, node_struct);
        else if (block->nodes.elems[i].type == ASTType_BLOCK) {
            create_stack_frame(instrs,
                    ((const struct BlockNode*)node_struct)->var_bytes);
            get_block_instructions(instrs, node_struct);
            destroy_stack_frame(instrs);
        }
        else if (block->nodes.elems[i].type == ASTType_RETURN) {
            get_ret_stmt_instructions(instrs, node_struct);
        }
        else if (block->nodes.elems[i].type == ASTType_IF_STMT) {
            get_if_stmt_instructions(instrs, node_struct);
        }

        else if (block->nodes.elems[i].type == ASTType_DEBUG_RAX) {
            struct Instruction debug_instr = Instruction_init();
            debug_instr.type = InstrType_DEBUG_EAX;
            InstrList_push_back(instrs, debug_instr);
        }

        else
            assert(false);

    }

}

struct InstrList IR_get_instructions(const struct BlockNode *ast) {

    struct InstrList instrs = InstrList_init();

    get_block_instructions(&instrs, ast);

    return instrs;

}

m_define_VectorImpl_funcs(InstrList, struct Instruction)
