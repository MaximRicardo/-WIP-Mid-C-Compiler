#pragma once

#include "transl_unit.h"
#include <stdio.h>

void TranslUnit_compile_via_mccir(struct TranslUnit *tu, FILE *mccir_output,
        FILE *asm_output);
