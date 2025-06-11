#include "code_gen.h"
#include "ir.h"
#include <assert.h>
#include <stdio.h>

const char *reg_names[][3] = {
    {"al", "ax", "eax"},
    {"bl", "bx", "ebx"},
    {"cl", "cx", "ecx"},
    {"spl", "sp", "esp"},
    {"bpl", "bp", "ebp"},
    {"sil", "si", "esi"},
    {"dil", "di", "edi"},
    {"dl", "dx", "edx"},
};

const char *dx_names[] = {
    "dl", "dx", "edx"
};

const char *si_names[] = {
    "sil", "si", "esi"
};

const char *di_names[] = {
    "dil", "di", "edi"
};

const char *instr_type_to_asm[] = {
    "INVALID INSTRUCTION",

    "mov",
    "mov",
    "mov",

    "lea",

    "xchg",

    "add",
    "sub",
    "mul",
    "imul",
    "div",
    "idiv",
    "div",
    "idiv",
    "and",
    "cmp",
    "shl",
    "shr",
    "ashr",

    "not",

    "push",
    "pop",
    "call",
    "ret",

    "jmp",
    "je",
    "jne",

    "LABEL INSTRUCTION",
};

/* used in stuff like pushing an immediate */
const char *size_specifier[] = {

    "byte",
    "word",
    "dword",

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

    return type == InstrType_ADD || type == InstrType_SUB ||
        type == InstrType_MOV || type == InstrType_AND ||
        type == InstrType_CMP;

}

static bool branch_instr(enum InstrType type) {

    return type == InstrType_JMP || type == InstrType_JE ||
        type == InstrType_JNE;

}

static bool shift_instr(enum InstrType type) {

    return type == InstrType_SHL || type == InstrType_SHR ||
        type == InstrType_ASHR;

}

static void write_instr(FILE *output, const struct Instruction *instr) {

    if (instr->type == InstrType_MOV_F_LOC) {
        assert(type_is_reg(instr->lhs.type));
        assert(type_is_reg(instr->rhs.type));

        fprintf(output, "mov %s",
                reg_names[type_to_reg(instr->lhs.type)][instr->instr_size]
                );

        fprintf(output, ", [%s+%d]\n",
                reg_names[type_to_reg(instr->rhs.type)][InstrSize_32],
                instr->offset
                );
    }

    else if (instr->type == InstrType_MOV_T_LOC) {
        assert(type_is_reg(instr->lhs.type));

        if (type_is_reg(instr->rhs.type)) {
            fprintf(output, "mov [%s+%d]",
                    reg_names[type_to_reg(instr->lhs.type)][InstrSize_32],
                    instr->offset
                    );

            fprintf(output, ", %s\n",
                    reg_names[type_to_reg(instr->rhs.type)][instr->instr_size]
                    );
        }
        else {
            printf("instr size = %d\n", instr->instr_size);
            fprintf(output, "mov %s [%s+%d]",
                    size_specifier[instr->instr_size],
                    reg_names[type_to_reg(instr->lhs.type)][InstrSize_32],
                    instr->offset
                    );

            fprintf(output, ", %u\n", instr->rhs.value.imm);
        }
    }

    else if (instr->type == InstrType_LEA) {
        assert(type_is_reg(instr->lhs.type));

        fprintf(output, "lea %s",
                reg_names[type_to_reg(instr->lhs.type)][InstrSize_32]);

        if (type_is_reg(instr->rhs.type)) {
            fprintf(output, ", [%s+%d]\n",
                    reg_names[type_to_reg(instr->rhs.type)][InstrSize_32],
                    instr->offset);
        }
        else {
            fprintf(output, ", [%d+%d]\n", instr->rhs.value.imm,
                    instr->offset);
        }
    }

    else if (instr->type == InstrType_XCHG) {
        assert(type_is_reg(instr->lhs.type));
        assert(type_is_reg(instr->rhs.type));

        fprintf(output, "xchg %s, %s\n",
                reg_names[type_to_reg(instr->lhs.type)][instr->instr_size],
                reg_names[type_to_reg(instr->rhs.type)][instr->instr_size]);
    }

    else if (instr->is_unary) {
        assert(type_is_reg(instr->lhs.type));
        fprintf(output, "%s %s\n", instr_type_to_asm[instr->type],
                reg_names[type_to_reg(instr->lhs.type)][instr->instr_size]);
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
            fprintf(output, ", %s %d\n", size_specifier[instr->instr_size],
                    instr->rhs.value.imm);
    }

    else if (instr->type == InstrType_MUL || instr->type == InstrType_IMUL) {
        /* why x86? just why? */
        if (instr->lhs.type != InstrOperandType_REG_AX)
            fprintf(output, "xchg rax, %s\n",
                    reg_names[type_to_reg(instr->lhs.type)][InstrSize_32]);

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
                    reg_names[type_to_reg(instr->lhs.type)][InstrSize_32]);
    }

    else if (instr->type == InstrType_DIV || instr->type == InstrType_IDIV) {
        /* day 7045205478 of questioning why div and mul always use AX */
        if (instr->lhs.type != InstrOperandType_REG_AX)
            fprintf(output, "xchg rax, %s\n",
                    reg_names[type_to_reg(instr->lhs.type)][InstrSize_32]);

        if (instr->type == InstrType_DIV)
            fprintf(output, "xor rdx, rdx\n");
        else {
            if (instr->instr_size == InstrSize_32)
                fprintf(output, "cdq\n");
            else if (instr->instr_size == InstrSize_32)
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
                    reg_names[type_to_reg(instr->lhs.type)][InstrSize_32]);
    }

    else if (instr->type == InstrType_MODULO ||
            instr->type == InstrType_IMODULO) {
        /* day 7045205478 of questioning why div and mul always use AX */
        if (instr->lhs.type != InstrOperandType_REG_AX)
            fprintf(output, "xchg rax, %s\n",
                    reg_names[type_to_reg(instr->lhs.type)][InstrSize_32]);

        if (instr->type == InstrType_DIV)
            fprintf(output, "xor rdx, rdx\n");
        else {
            if (instr->instr_size == InstrSize_32)
                fprintf(output, "cdq\n");
            else if (instr->instr_size == InstrSize_32)
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
                    reg_names[type_to_reg(instr->lhs.type)][InstrSize_32]);

        /* the remainder is in DX */
        fprintf(output, "mov %s, %s\n",
                reg_names[type_to_reg(instr->lhs.type)][instr->instr_size],
                dx_names[instr->instr_size]);
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

    else if (instr->type == InstrType_CALL) {
        assert(instr->string);
        fprintf(output, "call %s\n", instr->string);
    }

    else if (instr->type == InstrType_RET) {
        fprintf(output, "ret\n");
    }

    else if (branch_instr(instr->type)) {
        fprintf(output, "%s %s\n", instr_type_to_asm[instr->type],
            instr->string);
    }

    else if (instr->type == InstrType_LABEL) {
        assert(instr->string);
        fprintf(output, "%s:\n", instr->string);
    }

    else if (shift_instr(instr->type)) {
        printf("idx = %u\n", instr->lhs.type);
        fprintf(output, "%s %s", instr_type_to_asm[instr->type],
                reg_names[type_to_reg(instr->lhs.type)][instr->instr_size]);
        if (type_is_reg(instr->rhs.type))
            fprintf(output, ", %s\n",
                    reg_names[type_to_reg(instr->rhs.type)][InstrSize_8]);
        else
            fprintf(output, ", %u\n", instr->rhs.value.imm);
    }

    else if (instr->type == InstrType_DEBUG_EAX) {
        fprintf(output, "mov ebx, esp\n");
        fprintf(output, "and esp, -16\n");
        fprintf(output, "push eax\n");
        fprintf(output, "push msg$\n");
        fprintf(output, "call printf\n");
        fprintf(output, "mov esp, ebx\n");
    }

    else {
        fprintf(stderr, "invalid instruction %d.\n", instr->type);
        assert(false);
    }

}

void CodeGenArch_generate(FILE *output, const struct BlockNode *ast) {

    struct InstrList instrs = IR_get_instructions(ast);
    unsigned i;

    fprintf(output, "[BITS 32]\n\n");
    fprintf(output, "extern printf\n");
    fprintf(output, "\nsection .text\n");
    fprintf(output, "global main\n");

    for (i = 0; i < instrs.size; i++) {
        write_instr(output, &instrs.elems[i]);
        fprintf(output, "\n");
    }

    fprintf(output, "\nsection .rodata\n");
    fprintf(output, "msg$: db `result = %%d\\n\\0`\n");

    while (instrs.size > 0) {
        InstrList_pop_back(&instrs, Instruction_free);
    }
    InstrList_free(&instrs);

}
