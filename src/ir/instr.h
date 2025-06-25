#pragma once

#include "../comp_dependent/ints.h"
#include "../vector_impl.h"
#include "data_types.h"
#include "../ast.h"

enum IRInstrType {

    IRInstr_INVALID,

    IRInstr_MOV,

    IRInstr_ADD,
    IRInstr_SUB,
    IRInstr_MUL,
    IRInstr_DIV,

    /* control flow instructions */
    IRInstr_JMP,
    IRInstr_JE,
    IRInstr_RET,

    /* mem instructions */
    IRInstr_ALLOCA,
    IRInstr_STORE,
    IRInstr_LOAD,

    IRInstr_COMMENT

};

bool IRInstrType_is_branch(enum IRInstrType type);
bool IRInstrType_is_cond_branch(enum IRInstrType type);
/* not counting the result register */
bool IRInstrType_is_bin_op(enum IRInstrType type);

enum IRInstrArgType {

    IRInstrArg_INVALID,

    IRInstrArg_IMM32,
    IRInstrArg_REG,

    IRInstrArg_STR

};

union IRInstrArgValue {

    u32 imm_u32;
    i32 imm_i32;
    char *reg_name;
    char *generic_str;

};

union IRInstrArgValue IRInstrArgValue_imm_u32(u32 imm32);
union IRInstrArgValue IRInstrArgValue_imm_i32(i32 imm32);
union IRInstrArgValue IRInstrArgValue_reg_name(char *reg_name);
union IRInstrArgValue IRInstrArgValue_generic_str(char *str);

struct IRInstrArg {

    enum IRInstrArgType type;
    struct IRDataType data_type;
    union IRInstrArgValue value;

};

struct IRInstrArg IRInstrArg_init(void);
struct IRInstrArg IRInstrArg_create(enum IRInstrArgType type,
        struct IRDataType data_type, union IRInstrArgValue value);
void IRInstrArg_free(struct IRInstrArg arg);
/*
 * reg                - if the instr arg ends up using a register, this is the
 *                      name of the reg it should use. automatically gets freed
 *                      if it doesn't get used.
 */
struct IRInstrArg IRInstrArg_create_from_expr(const struct Expr *expr,
        const struct StructList *structs, char *reg);

struct IRInstrArgList {

    struct IRInstrArg *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(IRInstrArgList, struct IRInstrArg)

struct IRInstr {

    enum IRInstrType type;
    struct IRInstrArgList args;

};

struct IRInstr IRInstr_init(void);
struct IRInstr IRInstr_create(enum IRInstrType type,
        struct IRInstrArgList args);
void IRInstr_free(struct IRInstr instr);
struct IRDataType IRInstr_data_type(const struct IRInstr *self);

struct IRInstr IRInstr_create_mov(char *dest, struct IRDataType dest_d_type,
        struct IRInstrArg arg);

struct IRInstr IRInstr_create_str_instr(enum IRInstrType type, char *dest);
struct IRInstr IRInstr_create_cond_jmp_instr(enum IRInstrType type,
        struct IRInstrArg cond_lhs, struct IRInstrArg cond_rhs, char *dest);

struct IRInstrList {

    struct IRInstr *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(IRInstrList, struct IRInstr)
