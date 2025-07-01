#pragma once

#include "../module.h"

/* returns the generated x86 asm code as a regular string */
char* gen_x86_from_ir(struct IRModule *module);
