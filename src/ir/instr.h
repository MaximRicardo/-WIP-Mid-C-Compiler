#pragma once

#include "../comp_dependent/ints.h"
#include "../vector_impl.h"
#include "data_types.h"

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

struct IRInstrList {

    struct IRInstr *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(IRInstrList, struct IRInstr)
