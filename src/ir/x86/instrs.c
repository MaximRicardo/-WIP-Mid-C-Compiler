#include "instrs.h"
#include <assert.h>

const char* X86_get_instr(enum IRInstrType type, struct IRDataType d_type) {

    switch (type) {

    case IRInstr_INVALID:
        assert(false);

    case IRInstr_ADD:
        return "add";

    case IRInstr_SUB:
        return "sub";

    case IRInstr_MUL:
        return "imul";

    case IRInstr_DIV:
        return d_type.is_signed ? "idiv" : "div";

    case IRInstr_RET:
        return "ret";

    case IRInstr_JMP:
        return "jmp";

    case IRInstr_JE:
        return "je";

    case IRInstr_COMMENT:
        assert(false);

    default:
        assert(false);

    };

}
