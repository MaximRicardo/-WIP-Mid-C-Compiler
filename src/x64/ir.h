#pragma once

#include "../vector_impl.h"
#include "../ast.h"

/* when changing make sure to update:
 *  expr_t_to_instr_t() in ir.c, instr_type_to_asm
 *  in code_gen.c, regular_2_oper_instr in code_gen.c
 */
enum InstrType {

    InstrType_INVALID,

    InstrType_MOV,
    InstrType_MOV_F_LOC,  /* example: mov rax, [rbx+8] */
    InstrType_MOV_T_LOC,  /* example: mov [rbx+8], rax */

    InstrType_LEA,

    /* binary operations */
    InstrType_ADD,
    InstrType_SUB,
    InstrType_MUL,
    InstrType_IMUL,
    InstrType_DIV,
    InstrType_IDIV,
    InstrType_MODULO,
    InstrType_IMODULO,

    /* stack operations */
    InstrType_PUSH,
    InstrType_POP,
    InstrType_CALL,
    InstrType_RET,

    InstrType_LABEL,

    InstrType_DEBUG_RAX

};

enum InstrOperandType {

    InstrOperandType_INVALID,

    /* registers */
    InstrOperandType_REGISTERS_START,
    InstrOperandType_REG_AX,
    InstrOperandType_REG_BX,
    InstrOperandType_REG_CX,
    InstrOperandType_REG_R8,
    InstrOperandType_REG_R9,
    InstrOperandType_REG_R10,
    InstrOperandType_REG_R11,
    InstrOperandType_REG_R12,
    InstrOperandType_REG_R13,
    InstrOperandType_REG_R14,
    InstrOperandType_REG_R15,
    /* stack registers */
    InstrOperandType_REG_SP,
    InstrOperandType_REG_BP,
    /* index registers */
    InstrOperandType_REG_SI,
    InstrOperandType_REG_DI,
    /* misc. */
    InstrOperandType_REG_DX,
    InstrOperandType_REGISTERS_END,

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
    i32 offset;
    char *string;

};

struct Instruction Instruction_init(void);
void Instruction_free(struct Instruction instr);

struct InstrList {

    struct Instruction *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(InstrList, struct Instruction)

struct InstrList IR_get_instructions(const struct BlockNode *ast);
