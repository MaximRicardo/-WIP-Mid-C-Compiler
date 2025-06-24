#pragma once

#include "../comp_dependent/ints.h"
#include "../vector_impl.h"
#include "data_types.h"
#include "../ast.h"

enum IRInstrType {

    IRInstr_INVALID,

    IRInstr_ADD,
    IRInstr_SUB,
    IRInstr_MUL,
    IRInstr_DIV,

    IRInstr_COMMENT

};

enum IRInstrArgType {

    IRInstrArg_INVALID,

    IRInstrArg_IMM32,
    IRInstrArg_REG

};

union IRInstrArgValue {

    u32 imm_u32;
    i32 imm_i32;
    char *reg_name;

};

union IRInstrArgValue IRInstrArgValue_imm_u32(u32 imm32);
union IRInstrArgValue IRInstrArgValue_imm_i32(i32 imm32);
union IRInstrArgValue IRInstrArgValue_reg_name(char *reg_name);

struct IRInstrArg {

    enum IRInstrArgType type;
    struct IRDataType data_type;
    union IRInstrArgValue value;

};

struct IRInstrArg IRInstrArg_init(void);
struct IRInstrArg IRInstrArg_create(enum IRInstrArgType type,
        struct IRDataType data_type, union IRInstrArgValue value);
/*
 * reg                - if the instr arg ends up using a register, this is the
 *                      name of the reg it should use.
 */
struct IRInstrArg IRInstrArg_create_from_expr(const struct Expr *expr,
        const struct StructList *structs, char *reg);
void IRInstrArg_free(struct IRInstrArg arg);

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

struct IRInstrList {

    struct IRInstr *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(IRInstrList, struct IRInstr)
