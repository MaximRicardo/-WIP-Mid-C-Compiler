#pragma once

/* functions for generating an MCCIR program from an AST */

#include "../transl_unit.h"
#include "module.h"

struct IRModule IRGen_generate(const struct TranslUnit *tu);
