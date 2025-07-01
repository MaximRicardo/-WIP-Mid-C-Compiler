#pragma once

#include "../utils/bool.h"
#include "../prim_type.h"
#include "../structs.h"

/* the data type the instruction uses, like i32, u8*, etc. */
struct IRDataType {

    bool is_signed;
    /* measured in bits. can only be set to 8, 16 or 32 */
    u32 width;
    u32 lvls_of_indir;

};

struct IRDataType IRDataType_init(void);
struct IRDataType IRDataType_create(bool is_signed, u32 width,
        u32 lvls_of_indir);
struct IRDataType IRDataType_create_from_prim_type(enum PrimitiveType type,
        u32 type_idx, unsigned lvls_of_indir,
        const struct StructList *structs);
enum PrimitiveType IRDataType_to_prim_type(const struct IRDataType *self);
/* accounts for lvls of indir. ptrs are only 32 bits wide */
u32 IRDataType_real_width(const struct IRDataType *self);
