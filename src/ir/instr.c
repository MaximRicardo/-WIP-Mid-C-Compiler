#include "instr.h"
#include "data_types.h"
#include <stddef.h>
#include <assert.h>
#include <string.h>

bool IRInstrType_is_branch(enum IRInstrType type) {

    return type == IRInstr_JMP || type == IRInstr_JE;

}

bool IRInstrType_is_cond_branch(enum IRInstrType type) {

    return IRInstrType_is_branch(type) && type != IRInstr_JMP;

}

bool IRInstrType_is_bin_op(enum IRInstrType type) {

    return type > IRInstr_BINARY_OPS_START && type < IRInstr_BINARY_OPS_END;

}

bool IRInstrType_is_mem_instr(enum IRInstrType type) {

    return type > IRInstr_MEM_INSTRS_START && type < IRInstr_MEM_INSTRS_END;
}

bool IRInstrType_has_dest_reg(enum IRInstrType type) {

    return IRInstrType_is_bin_op(type) || type == IRInstr_LOAD ||
        type == IRInstr_ALLOCA || type == IRInstr_PHI ||
        type == IRInstr_MOV;

}

union IRInstrArgValue IRInstrArgValue_imm_u32(u32 imm32) {

    union IRInstrArgValue value;
    value.imm_u32 = imm32;
    return value;

}

union IRInstrArgValue IRInstrArgValue_imm_i32(i32 imm32) {

    union IRInstrArgValue value;
    value.imm_i32 = imm32;
    return value;

}

union IRInstrArgValue IRInstrArgValue_reg_name(const char *reg_name) {

    union IRInstrArgValue value;
    value.reg_name = reg_name;
    return value;

}

union IRInstrArgValue IRInstrArgValue_generic_str(const char *str) {

    union IRInstrArgValue value;
    value.generic_str = str;
    return value;

}

struct IRInstrArg IRInstrArg_init(void) {

    struct IRInstrArg x;
    x.type = IRInstrArg_INVALID;
    x.data_type = IRDataType_init();
    x.value.reg_name = NULL;
    return x;

}

struct IRInstrArg IRInstrArg_create(enum IRInstrArgType type,
        struct IRDataType data_type, union IRInstrArgValue value) {

    struct IRInstrArg x;
    x.type = type;
    x.data_type = data_type;
    x.value = value;
    return x;

}

struct IRInstrArg IRInstrArg_create_from_expr(const struct Expr *expr,
        const struct StructList *structs, const char *reg) {

    struct IRInstrArg x;

    x.type = expr->expr_type == ExprType_INT_LIT ?
        IRInstrArg_IMM32 : IRInstrArg_REG;

    x.data_type = IRDataType_create_from_prim_type(expr->prim_type,
            expr->type_idx, expr->lvls_of_indir, structs);

    x.value =
        x.type == IRInstrArg_REG ? IRInstrArgValue_reg_name(reg) :
        x.data_type.is_signed ? IRInstrArgValue_imm_i32(expr->int_value) :
        IRInstrArgValue_imm_u32(expr->int_value);

    return x;

}

struct IRInstr IRInstr_init(void) {

    struct IRInstr instr;
    instr.type = IRInstr_INVALID;
    instr.args = IRInstrArgList_init();
    return instr;

}

struct IRInstr IRInstr_create(enum IRInstrType type,
        struct IRInstrArgList args) {

    struct IRInstr instr;
    instr.type = type;
    instr.args = args;
    return instr;

}

void IRInstr_free(struct IRInstr instr) {

    while (instr.args.size > 0) {
        IRInstrArgList_pop_back(&instr.args, NULL);
    }
    IRInstrArgList_free(&instr.args);

}

struct IRDataType IRInstr_data_type(const struct IRInstr *self) {

    assert(self->args.size > 0);

    return self->args.elems[0].data_type;

}

struct IRInstr IRInstr_create_mov(const char *dest,
        struct IRDataType dest_d_type, struct IRInstrArg arg) {

    struct IRInstr instr = IRInstr_init();
    instr.type = IRInstr_MOV;

    IRInstrArgList_push_back(&instr.args, IRInstrArg_create(
                IRInstrArg_REG, dest_d_type, IRInstrArgValue_reg_name(dest)
                ));

    IRInstrArgList_push_back(&instr.args, arg);

    return instr;

}

struct IRInstr IRInstr_create_str_instr(enum IRInstrType type,
        const char *dest) {

    struct IRInstr instr = IRInstr_init();
    instr.type = type;
    IRInstrArgList_push_back(&instr.args,
            IRInstrArg_create(
                IRInstrArg_STR, IRDataType_init(),
                IRInstrArgValue_generic_str(dest)
                )
            );
    return instr;

}

struct IRInstr IRInstr_create_cond_jmp_instr(enum IRInstrType type,
        struct IRInstrArg cmp_lhs, struct IRInstrArg cmp_rhs,
        const char *true_dest, const char *false_dest) {

    struct IRInstr instr = IRInstr_init();
    instr.type = type;

    IRInstrArgList_push_back(&instr.args, cmp_lhs);
    IRInstrArgList_push_back(&instr.args, cmp_rhs);

    IRInstrArgList_push_back(&instr.args,
            IRInstrArg_create(
                IRInstrArg_STR, IRDataType_init(),
                IRInstrArgValue_generic_str(true_dest)
                )
            );

    IRInstrArgList_push_back(&instr.args,
            IRInstrArg_create(
                IRInstrArg_STR, IRDataType_init(),
                IRInstrArgValue_generic_str(false_dest)
                )
            );

    return instr;

}

struct IRInstr IRInstr_create_alloc_reg(const char *reg_name,
        struct IRDataType d_type) {

    struct IRInstr instr = IRInstr_init();
    instr.type = IRInstr_ALLOC_REG;

    IRInstrArgList_push_back(&instr.args, IRInstrArg_create(
                IRInstrArg_REG, d_type, IRInstrArgValue_reg_name(reg_name)
                ));

    return instr;

}

struct IRInstr IRInstr_create_alloca(const char *dest_vreg,
        struct IRDataType d_type, u32 size, u32 alignment) {

    struct IRDataType u32_d_type = IRDataType_create(false, 32, 0);

    struct IRInstr instr = IRInstr_init();
    instr.type = IRInstr_ALLOCA;

    IRInstrArgList_push_back(&instr.args, IRInstrArg_create(
                IRInstrArg_REG, d_type, IRInstrArgValue_reg_name(dest_vreg)
                ));

    IRInstrArgList_push_back(&instr.args, IRInstrArg_create(
                IRInstrArg_IMM32, u32_d_type, IRInstrArgValue_imm_u32(size)
                ));

    IRInstrArgList_push_back(&instr.args, IRInstrArg_create(
                IRInstrArg_IMM32, u32_d_type, IRInstrArgValue_imm_u32(alignment)
                ));

    return instr;

}

m_define_VectorImpl_funcs(IRInstrArgList, struct IRInstrArg)
m_define_VectorImpl_funcs(IRInstrList, struct IRInstr)
