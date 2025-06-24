#include "data_types.h"

struct IRDataType IRDataType_init(void) {

    struct IRDataType x;
    x.is_signed = false;
    x.width = 0;
    x.lvls_of_indir = 0;
    return x;

}

struct IRDataType IRDataType_create(bool is_signed, unsigned width,
        unsigned lvls_of_indir) {

    struct IRDataType x;
    x.is_signed = is_signed;
    x.width = width;
    x.lvls_of_indir = lvls_of_indir;
    return x;

}
