#pragma once

/* converts every phi node into one vreg. for example (pseudo code):
 *
 *    x.0 := 5
 *    if (condition)
 *      x.1 := x.0+y
 *    else
 *      x.2 := x.0-y
 *    x.3 = phi(x.1, x.2)
 *    return x.3
 *
 * would get converted to:
 *
 *    x.0 := 5
 *    if (condition)
 *      x.1 := x.0+y
 *    else
 *      x.1 := x.0-y
 *    return x.1;
 *
 * this would be done at the very end of the compilation pipeline, right before
 * converting to assembly.
 * */

#include "module.h"

void IR_ssa_to_tac(struct IRModule *module);
