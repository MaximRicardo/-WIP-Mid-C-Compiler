#pragma once

/* arguments that have been passed to the compiler */

#include "bool.h"

struct CompArgs {

    const char *src_path;
    const char *mccir_out_path;
    const char *asm_out_path;

    bool optimize;
    /* WARNING: TEMPORARY OPTION TO MAKE DEBUGGING THE MCCIR BACKEND EASIER,
     * BUT WILL CRASH THE COMPILER IF SIZEOF IS USED ANYWHERE IN THE COMPILED
     * CODE */
    bool no_const_folding;
    bool w_error;
    bool pedantic;

};

extern struct CompArgs CompArgs_args;

struct CompArgs CompArgs_init(void);
struct CompArgs CompArgs_get_args(int argc, char **argv);
