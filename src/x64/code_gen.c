#include "code_gen.h"
#include "ir.h"
#include <assert.h>
#include <stdio.h>

const char *reg_names[][4] = {
    {"al", "ax", "eax", "rax"},
    {"bl", "bx", "ebx", "rbx"},
    {"cl", "cx", "ecx", "rcx"},
    {"r8b", "r8w", "r8d", "r8"},
    {"r9b", "r9w", "r9d", "r9"},
    {"r10b", "r10w", "r10d", "r10"},
    {"r11b", "r11w", "r11d", "r11"},
    {"r12b", "r12w", "r12d", "r12"},
    {"r13b", "r13w", "r13d", "r13"},
    {"r14b", "r14w", "r14d", "r14"},
    {"r15b", "r15w", "r15d", "r15"},
    {"bpl", "bp", "ebp", "rbp"},
    {"spl", "sp", "esp", "rsp"},
};

const char *dx_names[] = {
    "dl", "dx", "edx", "rdx"
};

const char *si_names[] = {
    "sil", "si", "esi", "rsi"
};

const char *di_names[] = {
    "dil", "di", "edi", "rdi"
};

const char *instr_type_to_asm[] = {
    "INVALID INSTRUCTION",

    "mov",

    "add",
    "sub",
    "mul",
    "imul",
    "div",
    "idiv",

    "push",
    "pop",
};

/* used in stuff like pushing an immediate */
const char *size_specifier[] = {

    "byte",
    "word",
    "dword",
    "qword",

};

static unsigned type_to_reg(enum InstrOperandType type) {

    return type-InstrOperandType_REGISTERS_START-1;

}

static bool type_is_reg(enum InstrOperandType type) {

    return type > InstrOperandType_REGISTERS_START &&
        type < InstrOperandType_REGISTERS_END;

}

/* is the instruction type one that can take any register as the first operand
 * and any register or an immediate as the second argument? */
static bool regular_2_oper_instr(enum InstrType type) {

    return type == InstrType_ADD || type == InstrType_SUB;

}

static void write_instr(FILE *output, const struct Instruction *instr) {

    if (instr->type == InstrType_MOV) {
        if (type_is_reg(instr->lhs.type))
            fprintf(output, "mov %s",
                    reg_names[type_to_reg(instr->lhs.type)][instr->instr_size]
                    );
        else {
            printf("type = %d\n", instr->lhs.type);
            assert(false);
        }

        if (type_is_reg(instr->rhs.type))
            fprintf(output, ", %s\n",
                    reg_names[type_to_reg(instr->rhs.type)][instr->instr_size]
                    );
        else
            fprintf(output, ", %u\n", instr->rhs.value.imm);
    }
    else if (regular_2_oper_instr(instr->type)) {
        assert(type_is_reg(instr->lhs.type));
        fprintf(output, "%s %s", instr_type_to_asm[instr->type],
                reg_names[type_to_reg(instr->lhs.type)][instr->instr_size]
                );

        if (type_is_reg(instr->rhs.type))
            fprintf(output, ", %s\n",
                    reg_names[type_to_reg(instr->rhs.type)][instr->instr_size]
                    );
        else
            fprintf(output, ", %d\n", instr->rhs.value.imm);
    }
    else if (instr->type == InstrType_MUL || instr->type == InstrType_IMUL) {
        /* why x86? just why? */
        if (instr->lhs.type != InstrOperandType_REG_AX)
            fprintf(output, "xchg rax, %s\n",
                    reg_names[type_to_reg(instr->lhs.type)][InstrSize_64]);

        if (type_is_reg(instr->rhs.type))
            fprintf(output, "%s %s\n", instr_type_to_asm[instr->type],
                    reg_names[type_to_reg(instr->rhs.type)][instr->instr_size]
                    );
        else {
            fprintf(output, "mov %s, %u\n", dx_names[instr->instr_size],
                    instr->rhs.value.imm);
            fprintf(output, "%s %s\n", instr_type_to_asm[instr->type],
                    dx_names[instr->instr_size]);
        }

        if (instr->lhs.type != InstrOperandType_REG_AX)
            fprintf(output, "xchg rax, %s\n",
                    reg_names[type_to_reg(instr->lhs.type)][InstrSize_64]);
    }
    else if (instr->type == InstrType_DIV || instr->type == InstrType_IDIV) {
        /* day 7045205478 of questioning why div and mul always use AX */
        if (instr->lhs.type != InstrOperandType_REG_AX)
            fprintf(output, "xchg rax, %s\n",
                    reg_names[type_to_reg(instr->lhs.type)][InstrSize_64]);

        if (instr->type == InstrType_DIV)
            fprintf(output, "xor rdx, rdx\n");
        else {
            if (instr->instr_size == InstrSize_32)
                fprintf(output, "cdq\n");
            else if (instr->instr_size == InstrSize_64)
                fprintf(output, "cqo\n");
            else
                assert(false);
        }

        if (type_is_reg(instr->rhs.type))
            fprintf(output, "%s %s\n", instr_type_to_asm[instr->type],
                    reg_names[type_to_reg(instr->rhs.type)][instr->instr_size]
                    );
        else {
            /* uses SI to temporarily hold the immediate value */
            fprintf(output, "mov %s, %u\n", si_names[instr->instr_size],
                    instr->rhs.value.imm);
            fprintf(output, "%s %s\n", instr_type_to_asm[instr->type],
                    si_names[instr->instr_size]);
        }

        if (instr->lhs.type != InstrOperandType_REG_AX)
            fprintf(output, "xchg rax, %s\n",
                    reg_names[type_to_reg(instr->lhs.type)][InstrSize_64]);
    }
    else if (instr->type == InstrType_PUSH) {
        if (type_is_reg(instr->lhs.type))
            fprintf(output, "push %s\n",
                    reg_names[type_to_reg(instr->lhs.type)][instr->instr_size]
                    );
        else
            fprintf(output, "push %s %u\n", size_specifier[instr->instr_size],
                    instr->lhs.value.imm);
    }
    else if (instr->type == InstrType_POP) {
        assert(type_is_reg(instr->lhs.type));
        fprintf(output, "pop %s\n",
                reg_names[type_to_reg(instr->lhs.type)][instr->instr_size]);
    }
    else
        assert(false);

}

void CodeGenArch_generate(FILE *output, const struct TUNode *tu) {

    struct InstrList instrs = Instruction_get_instructions(tu);
    unsigned i;

    fprintf(output, "[BITS 64]\n\n");
    fprintf(output, "extern printf\n");
    fprintf(output, "\nsection .text\n");
    fprintf(output, "\nglobal main\nmain:\n");
    fprintf(output, "push rax\n");  /* Align the stack to 16 bytes */

    for (i = 0; i < instrs.size; i++) {
        write_instr(output, &instrs.elems[i]);
        fprintf(output, "\n");
    }

    fprintf(output, "mov rbx, rax\n");
    fprintf(output, "mov al, 0\n");
    fprintf(output, "mov rdi, msg\n");
    fprintf(output, "mov rsi, rbx\n");
    fprintf(output, "call printf\n\n");

    fprintf(output, "pop rax\n");
    fprintf(output, "ret\n");

    fprintf(output, "\nsection .rodata\n");
    fprintf(output, "msg: db `result = %%d\\n\\0`\n");

    InstrList_free(&instrs);

}
