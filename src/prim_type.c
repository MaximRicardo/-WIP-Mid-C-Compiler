#include "prim_type.h"
#include "backend_dependent/type_sizes.h"
#include <assert.h>

bool PrimitiveType_signed(enum PrimitiveType type, unsigned lvls_of_indir) {

    return lvls_of_indir == 0 && (
            type == PrimType_INT || type == PrimType_CHAR ||
            type == PrimType_SHORT || type == PrimType_LONG
            );

}

unsigned PrimitiveType_size(enum PrimitiveType type, unsigned lvls_of_indir) {

    if (lvls_of_indir > 0)
        return m_TypeSize_ptr;

    switch (type) {

    case PrimType_CHAR:
    case PrimType_UCHAR:
        return m_TypeSize_char;

    case PrimType_SHORT:
    case PrimType_USHORT:
        return m_TypeSize_short;

    case PrimType_INT:
    case PrimType_UINT:
        return m_TypeSize_int;

    case PrimType_LONG:
    case PrimType_ULONG:
        return m_TypeSize_long;

    case PrimType_VOID:
    case PrimType_INVALID:
        assert(false);

    }

}

enum PrimitiveType PrimitiveType_promote(enum PrimitiveType type,
        unsigned lvls_of_indir) {

    if (lvls_of_indir > 0)
        return type;

    switch (type) {

    case PrimType_CHAR:
    case PrimType_SHORT:
    case PrimType_INT:
        return PrimType_INT;

    case PrimType_UCHAR:
        if (m_TypeSize_s_int_can_hold_uchar)
            return PrimType_INT;
        else
            return PrimType_UINT;

    case PrimType_USHORT:
        if (m_TypeSize_s_int_can_hold_ushort)
            return PrimType_INT;
        else
            return PrimType_UINT;

    case PrimType_UINT:
        return PrimType_UINT;

    case PrimType_LONG:
        return PrimType_LONG;

    case PrimType_ULONG:
        return PrimType_ULONG;

    case PrimType_VOID:
    case PrimType_INVALID:
        assert(false);

    }

}

enum PrimitiveType PrimitiveType_make_unsigned(enum PrimitiveType type) {

    switch (type) {

    case PrimType_CHAR:
    case PrimType_UCHAR:
        return PrimType_UCHAR;

    case PrimType_SHORT:
    case PrimType_USHORT:
        return PrimType_USHORT;

    case PrimType_INT:
    case PrimType_UINT:
        return PrimType_UINT;

    case PrimType_LONG:
    case PrimType_ULONG:
        return PrimType_ULONG;

    default:
        assert(false);

    }

}
