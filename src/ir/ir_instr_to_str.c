#include "ir_instr_to_str.h"
#include "instr.h"
#include <assert.h>

const char *IR_instr_t_to_str(enum IRInstrType type) {

    switch (type) {

    case IRInstr_INVALID:
        assert(false);

    case IRInstr_MOV:
        return "mov";

    case IRInstr_BINARY_OPS_START:
        assert(false);

    case IRInstr_ADD:
        return "add";

    case IRInstr_SUB:
        return "sub";

    case IRInstr_MUL:
        return "mul";

    case IRInstr_DIV:
        return "div";

    case IRInstr_SET_EQ:
        return "seteq";

    case IRInstr_SET_NEQ:
        return "setneq";

    case IRInstr_SET_LT:
        return "setlt";

    case IRInstr_SET_LTEQ:
        return "setlteq";

    case IRInstr_SET_GT:
        return "setgt";

    case IRInstr_SET_GTEQ:
        return "setgteq";

    case IRInstr_BINARY_OPS_END:
        assert(false);

    case IRInstr_JMP:
        return "jmp";

    case IRInstr_JE:
        return "je";

    case IRInstr_CALL:
        return "call";

    case IRInstr_RET:
        return "ret";

    case IRInstr_MEM_INSTRS_START:
        assert(false);

    case IRInstr_ALLOCA:
        return "alloca";

    case IRInstr_STORE:
        return "store";

    case IRInstr_LOAD:
        return "load";

    case IRInstr_MEM_INSTRS_END:
        assert(false);

    case IRInstr_PHI:
        return "phi";

    case IRInstr_ALLOC_REG:
        return "alloc_reg";

    case IRInstr_COMMENT:
        assert(false);

    }

}
