#pragma once

#include "bool.h"
#include "comp_dependent/ints.h"

struct StructList;

enum PrimitiveType {

    PrimType_INVALID,

    PrimType_CHAR,
    PrimType_UCHAR,

    PrimType_SHORT,
    PrimType_USHORT,

    PrimType_INT,
    PrimType_UINT,

    PrimType_LONG,
    PrimType_ULONG,

    PrimType_VOID,

    PrimType_STRUCT

};

bool PrimitiveType_signed(enum PrimitiveType type, unsigned lvls_of_indir);
/* type_idx is used by structs and unions and such. */
unsigned PrimitiveType_size(enum PrimitiveType type, unsigned lvls_of_indir,
        u32 type_idx, const struct StructList *structs);
enum PrimitiveType PrimitiveType_promote(enum PrimitiveType type,
        unsigned lvls_of_indir);
enum PrimitiveType PrimitiveType_make_unsigned(enum PrimitiveType type);
/* structs, unions and enums */
bool PrimitiveType_non_prim_type(enum PrimitiveType type);
bool PrimitiveType_can_convert_to(enum PrimitiveType conv_dest,
        unsigned dest_indir, u32 dest_type_idx, enum PrimitiveType conv_src,
        unsigned src_indir, u32 src_type_idx);
