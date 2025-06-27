#pragma once

#include "func.h"

/* gets the immediate dominator of every block in func */
void IRFunc_get_blocks_imm_doms(struct IRFunc *func);
