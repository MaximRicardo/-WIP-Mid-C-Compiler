#include "code_gen.h"
#include "ir.h"
#include <assert.h>
#include <stdio.h>

/* ts is honestly so fucking cooked ngl gang. normally i would look into
 * refactoring this code, but this x86 backend is only temporary, so im not
 * gonna bother. */

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

    "BINARY OPERATORS START",
    "add",
    "sub",
    "mul",
    "imul",
    "div",
    "idiv",
    "div",
    "idiv",
    "and",
    "or",
    "cmp",
    "shl",
    "shr",
    "ashr",
    "BINARY OPERATORS END",

    "UNARY OPERATORS START",
    "not",
    "neg",  /* hmm */
    "sete",
    "setne",
    "setl",
    "setle",
    "setg",
    "setge",
    "setb",
    "setbe",
    "seta",
    "setae",
    "inc",
    "dec",
    "inc",
    "dec",
    "UNARY OPERATORS END",

    "push",
    "pop",
    "call",
    "ret",

    "jmp",
    "je",
    "jne",

    "LABEL INSTRUCTION",

    "extern",
    "global",
};

/* used in stuff like pushing an immediate */
const char *size_specifier[] = {

    "byte",
    "word",
    "dword",

};

const char *elem_size_specifier[] = {

    "db",
    "dw",
    "dd",

};

/* returns the log2 of bytes. only works for 1, 2, and 4 bytes. i can't get
 * log2 and friends to work for some reason, my best guess is that c89 doesn't
 * support them? */
static unsigned bytes_log2(unsigned bytes) {

    switch (bytes) {

    case 1:
        return 0;

    case 2:
        return 1;

    case 4:
        return 2;

    default:
        assert(false);

    }

}

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
        type == InstrType_OR || type == InstrType_CMP;

}

static bool unary_instr(enum InstrType type) {

    return type > InstrType_UNARY_START && type < InstrType_UNARY_END;

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

    else if (instr->type == InstrType_INC_LOC ||
            instr->type == InstrType_DEC_LOC) {
        if (type_is_reg(instr->lhs.type))
            fprintf(output, "%s %s [%s]\n", instr_type_to_asm[instr->type],
                    size_specifier[instr->instr_size],
                    reg_names[type_to_reg(instr->lhs.type)][InstrSize_32]
                    );
        else if (instr->string)
            fprintf(output, "%s %s [%s]\n", instr_type_to_asm[instr->type],
                    size_specifier[instr->instr_size], instr->string);
        else
            fprintf(output, "%s %s [%u]\n", instr_type_to_asm[instr->type],
                    size_specifier[instr->instr_size], instr->lhs.value.imm);
    }

    else if (unary_instr(instr->type)) {
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
        else if (instr->string) {
            fprintf(output, ", %s\n", instr->string);
        }
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
        else if (instr->string)
            fprintf(output, "push %s\n", instr->string);
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

    else if (instr->type == InstrType_EXTERN ||
            instr->type == InstrType_GLOBAL) {
        assert(instr->string);
        fprintf(output, "%s %s\n", instr_type_to_asm[instr->type],
                instr->string);
    }

    else if (shift_instr(instr->type)) {
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

void CodeGenArch_generate(FILE *output, const struct BlockNode *ast,
        const struct StructList *structs) {

    struct ArrayLitList array_lits = ArrayLitList_init();

    struct InstrList instrs = IR_get_instructions(ast, structs);
    u32 i;

    BlockNode_get_array_lits(ast, &array_lits);

    fprintf(output, "[BITS 32]\n\n");
    fprintf(output, "extern memcpy\n");
    fprintf(output, "extern printf\n");
    fprintf(output, "\nsection .text\n");
    fprintf(output, "global main\n");

    for (i = 0; i < instrs.size; i++) {
        write_instr(output, &instrs.elems[i]);
        fprintf(output, "\n");
    }

    fprintf(output, "\nsection .rodata\n");
    fprintf(output, "msg$: db `result = %%d\\n\\0`\n");

    for (i = 0; i < array_lits.size; i++) {
        u32 j;
        fprintf(output, "array_lit_%lu$: %s ", (unsigned long)i,
                elem_size_specifier[
                    bytes_log2(array_lits.elems[i].elem_size)
                ]);
        for (j = 0; j < array_lits.elems[i].n_values; j++) {
            if (j != 0)
                fprintf(output, ", ");
            fprintf(output, "%d",
                    Expr_evaluate(array_lits.elems[i].values[j]));
        }
        fprintf(output, "\n");
    }

    while (instrs.size > 0) {
        InstrList_pop_back(&instrs, Instruction_free);
    }
    InstrList_free(&instrs);

    /* don't free the individual elements cuz they'll be freed when the ast is
     * freed. */
    ArrayLitList_free(&array_lits);

}
