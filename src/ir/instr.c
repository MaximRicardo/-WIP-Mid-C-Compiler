#include "instr.h"
#include "data_types.h"
#include <stddef.h>
#include <assert.h>

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

union IRInstrArgValue IRInstrArgValue_reg_name(char *reg_name) {

    union IRInstrArgValue value;
    value.reg_name = reg_name;
    return value;

}

union IRInstrArgValue IRInstrArgValue_generic_str(char *str) {

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
        const struct StructList *structs, char *reg) {

    struct IRInstrArg x;

    x.type = expr->expr_type == ExprType_INT_LIT ?
        IRInstrArg_IMM32 : IRInstrArg_REG;

    x.data_type = IRDataType_create_from_prim_type(expr->prim_type,
            expr->type_idx, expr->lvls_of_indir, structs);

    x.value =
        x.type == IRInstrArg_REG ? IRInstrArgValue_reg_name(reg) :
        x.data_type.is_signed ? IRInstrArgValue_imm_i32(expr->int_value) :
        IRInstrArgValue_imm_u32(expr->int_value);

    if (x.type != IRInstrArg_REG)
        m_free(reg);

    return x;

}

void IRInstrArg_free(struct IRInstrArg arg) {

    if (arg.type == IRInstrArg_REG)
        m_free(arg.value.reg_name);
    else if (arg.type == IRInstrArg_STR)
        m_free(arg.value.generic_str);

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
        IRInstrArgList_pop_back(&instr.args, IRInstrArg_free);
    }
    IRInstrArgList_free(&instr.args);

}

struct IRDataType IRInstr_data_type(const struct IRInstr *self) {

    assert(self->args.size > 0);

    return self->args.elems[0].data_type;

}

struct IRInstr IRInstr_create_str_instr(enum IRInstrType type, char *dest) {

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
        struct IRInstrArg cmp_lhs, struct IRInstrArg cmp_rhs, char *dest) {

    struct IRInstr instr = IRInstr_init();
    instr.type = type;

    IRInstrArgList_push_back(&instr.args, cmp_lhs);
    IRInstrArgList_push_back(&instr.args, cmp_rhs);

    IRInstrArgList_push_back(&instr.args,
            IRInstrArg_create(
                IRInstrArg_STR, IRDataType_init(),
                IRInstrArgValue_generic_str(dest)
                )
            );
    return instr;

}

struct IRInstr IRInstr_create_je(struct IRInstrArg cmp_lhs,
        struct IRInstrArg cmp_rhs, const char *dest);

m_define_VectorImpl_funcs(IRInstrArgList, struct IRInstrArg)
m_define_VectorImpl_funcs(IRInstrList, struct IRInstr)
