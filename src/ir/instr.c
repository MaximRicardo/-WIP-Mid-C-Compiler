#pragma once

#include "instr.h"
#include "data_types.h"
#include <stddef.h>

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

void IRInstrArg_free(struct IRInstrArg arg) {

    if (arg.type == IRInstrArg_REG)
        m_free(arg.value.reg_name);

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

m_define_VectorImpl_funcs(IRInstrArgList, struct IRInstrArg)
m_define_VectorImpl_funcs(IRInstrList, struct IRInstr)
