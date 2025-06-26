#pragma once

#include "module.h"

/* replaces alloca with virtual registers */
void IROpt_alloca(struct IRModule *module);
