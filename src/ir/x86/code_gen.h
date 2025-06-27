#pragma once

#include "../module.h"
#include "../../transl_unit.h"

/* returns the generated x86 asm code as a regular string */
char* gen_x86_from_ir(struct IRModule *module, const struct TranslUnit *tu);
