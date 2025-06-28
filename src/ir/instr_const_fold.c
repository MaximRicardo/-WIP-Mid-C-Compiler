#include "data_types.h"
#include "instr.h"
#include "../macros.h"

#define m_bin_oper(operation) \
    do { \
        if (is_signed) \
            result = self->args.elems[Arg_LHS].value.imm_i32 operation \
                self->args.elems[Arg_RHS].value.imm_i32; \
        else \
            result = self->args.elems[Arg_LHS].value.imm_u32 operation \
                self->args.elems[Arg_RHS].value.imm_u32; \
    } while (0)

bool IRInstr_const_fold(struct IRInstr *self) {

    u32 i;
    u32 result;
    bool is_signed = IRInstr_data_type(self).is_signed;
    const char *self_dest = self->args.elems[Arg_SELF].value.reg_name;
    struct IRDataType dest_d_type = self->args.elems[Arg_SELF].data_type;
    u32 max_lvls_of_indir = 0;

    for (i = 1; i < self->args.size; i++) {
        if (self->args.elems[i].type != IRInstrArg_IMM32)
            return false;
        max_lvls_of_indir = m_max(max_lvls_of_indir,
                    self->args.elems[i].data_type.lvls_of_indir);
    }

    switch (self->type) {

    case IRInstr_ADD:
        m_bin_oper(+);
        break;

    case IRInstr_SUB:
        m_bin_oper(-);
        break;

    case IRInstr_MUL:
        m_bin_oper(*);
        break;

    case IRInstr_DIV:
        m_bin_oper(/);
        break;

    default:
        return false;

    }

    /* convert self to a mov instr with an immediate operand */

    IRInstr_free(*self);

    *self = IRInstr_create_mov(self_dest, dest_d_type, IRInstrArg_create(
                IRInstrArg_IMM32,
                IRDataType_create(is_signed, 32, max_lvls_of_indir),
                IRInstrArgValue_imm_u32(result)
                ));

    return true;

}
