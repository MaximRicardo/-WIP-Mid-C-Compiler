#pragma once

/* arguments that have been passed to the compiler */

#include "bool.h"

struct CompArgs {

    const char *src_path;
    const char *asm_out_path;

    bool optimize;
    bool w_error;
    bool pedantic;

};

extern struct CompArgs CompArgs_args;

struct CompArgs CompArgs_init(void);
struct CompArgs CompArgs_get_args(int argc, char **argv);
