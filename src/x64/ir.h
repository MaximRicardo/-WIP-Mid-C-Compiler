#pragma once

#include "../vector_impl.h"
#include "../ast.h"

enum InstrType {

    InstrType_INVALID,

    InstrType_MOV,

    /* binary operations */
    InstrType_ADD,
    InstrType_SUB,
    InstrType_MUL,
    InstrType_IMUL,
    InstrType_DIV,
    InstrType_IDIV,

    /* stack operations */
    InstrType_PUSH,
    InstrType_POP

};

enum InstrOperandType {

    InstrOperandType_INVALID,

    /* registers */
    InstrOperandType_REGISTERS_START,
    InstrOperandType_REG_AX,
    InstrOperandType_REG_BX,
    InstrOperandType_REG_CX,
    /* Removing DX for now to make mul and div easier to impl */
    /* InstrOperandType_REG_DX,*/
    InstrOperandType_REG_R8,
    InstrOperandType_REG_R9,
    InstrOperandType_REG_R10,
    InstrOperandType_REG_R11,
    InstrOperandType_REG_R12,
    InstrOperandType_REG_R13,
    InstrOperandType_REG_R14,
    InstrOperandType_REG_R15,
    InstrOperandType_REGISTERS_END,

    InstrOperandType_REG_SP,
    InstrOperandType_REG_BP,

    /* immediates */
    InstrOperandType_IMM_8,
    InstrOperandType_IMM_16,
    InstrOperandType_IMM_32

};

struct InstrOperand {

    enum InstrOperandType type;
    union InstrOperandVal {
        u32 imm;
    } value;

};

struct InstrOperand InstrOperand_init(void);
struct InstrOperand InstrOperand_create(enum InstrOperandType type,
        union InstrOperandVal value);
struct InstrOperand InstrOperand_create_imm(enum InstrOperandType type,
        u32 imm);

enum InstrSize {

    InstrSize_8,
    InstrSize_16,
    InstrSize_32,
    InstrSize_64

};

struct Instruction {

    struct InstrOperand lhs, rhs;
    enum InstrType type;
    enum InstrSize instr_size;

};

struct Instruction Instruction_init(void);

struct InstrList {

    struct Instruction *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(InstrList, struct Instruction)

struct InstrList Instruction_get_instructions(const struct TUNode *tu);
