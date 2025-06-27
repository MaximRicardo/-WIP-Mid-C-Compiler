#pragma once

/* conversion from virtual registers to physical registers / stack offsets.
 * also calls X86_remove_alloc_reg */

#include "../module.h"

void X86_virt_to_phys(struct IRModule *module);
