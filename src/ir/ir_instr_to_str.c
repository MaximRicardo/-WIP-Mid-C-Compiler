#include "ir_instr_to_str.h"
#include "instr.h"

const char *IR_instr_t_to_str(enum IRInstrType type) {

    switch (type) {

    case IRInstr_INVALID:
        return "INVALID INSTRUCTION!";

    case IRInstr_ADD:
        return "add";

    case IRInstr_SUB:
        return "sub";

    case IRInstr_MUL:
        return "mul";

    case IRInstr_DIV:
        return "div";

    case IRInstr_JMP:
        return "jmp";

    case IRInstr_JE:
        return "je";

    case IRInstr_RET:
        return "ret";

    case IRInstr_COMMENT:
        return "COMMENT INSTRUCTION!";

    }

}
