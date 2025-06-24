#pragma once

#include "../bool.h"
#include "../prim_type.h"
#include "../structs.h"

/* the data type the instruction uses, like i32, u8*, etc. */
struct IRDataType {

    bool is_signed;
    /* measured in bits. can only be set to 8, 16 or 32 */
    unsigned width;
    unsigned lvls_of_indir;

};

struct IRDataType IRDataType_init(void);
struct IRDataType IRDataType_create(bool is_signed, unsigned width,
        unsigned lvls_of_indir);
struct IRDataType IRDataType_create_from_prim_type(enum PrimitiveType type,
        u32 type_idx, unsigned lvls_of_indir,
        const struct StructList *structs);
