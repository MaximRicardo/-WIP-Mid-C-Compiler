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

struct IRDataType IRDataType_create_from_prim_type(enum PrimitiveType type,
        u32 type_idx, unsigned lvls_of_indir,
        const struct StructList *structs) {

    struct IRDataType x;
    x.is_signed = PrimitiveType_signed(type, lvls_of_indir);
    x.lvls_of_indir = lvls_of_indir;
    x.width = PrimitiveType_size(type, lvls_of_indir, type_idx, structs)*8;
    return x;

}
