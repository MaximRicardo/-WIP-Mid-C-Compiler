#pragma once

#include "../module.h"
#include "../../transl_unit.h"

/* returns the generated MidAsm code as a regular string */
char* gen_Mid_Asm_from_ir(const struct IRModule *module,
        const struct TranslUnit *tu);
