#pragma once

#include "../instr.h"
#include "../data_types.h"

/* converts from IRInstrType and IRDataType to a single MidAsm opcode. */
const char* MidAsm_get_instr(enum IRInstrType type, struct IRDataType d_type);
