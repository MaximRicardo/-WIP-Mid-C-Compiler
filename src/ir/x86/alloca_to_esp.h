#pragma once

/* convert alloca instructions into stack ptr offsets.
 *    alloca i32* %0, i32 4, i32 4
 *    store i32 123, i32* %0, i32 0
 * becomes:
 *    alloca %__none, 4, 4
 *    store i32 123, i32* %__esp(0)&, 0
 * where %__esp(x)& is equivalent to esp+x. not to be confused with
 * %__esp(x), which is equivalent to *(esp+x).
 * the offset doesn't get merged into the offset of the store operation, due to
 * that not being possible if the offset were a runtime value.
 */

#include "../module.h"

void X86_alloca_to_esp(struct IRModule *module);
