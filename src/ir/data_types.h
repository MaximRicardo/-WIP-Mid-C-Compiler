#pragma once

#include "../bool.h"

/* the data type the instruction uses, like i32, u8*, etc. */
struct IRDataType {

    bool is_signed;
    unsigned width;
    unsigned lvls_of_indir;

};

struct IRDataType IRDataType_init(void);
struct IRDataType IRDataType_create(bool is_signed, unsigned width,
        unsigned lvls_of_indir);
