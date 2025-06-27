#pragma once

#include "../module.h"

/* removes all alloc_reg instructions. should only be done after the
 * virt_to_phys pass. */

void X86_remove_alloc_reg(struct IRModule *module);
