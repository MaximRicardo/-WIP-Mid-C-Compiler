#pragma once

#include "bool.h"

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

    PrimType_VOID

};

bool PrimitiveType_signed(enum PrimitiveType type, unsigned lvls_of_indir);
unsigned PrimitiveType_size(enum PrimitiveType type, unsigned lvls_of_indir);
enum PrimitiveType PrimitiveType_promote(enum PrimitiveType type,
        unsigned lvls_of_indir);
enum PrimitiveType PrimitiveType_make_unsigned(enum PrimitiveType type);
