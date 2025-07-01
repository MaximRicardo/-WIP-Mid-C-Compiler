#pragma once

#include "../module.h"

/* converts every use of a function arg into a stack ptr offset. using
 * the %__esp(x)& syntax. see alloca_to_esp.h for move information. */
void X86_arg_to_esp(struct IRModule *module);
