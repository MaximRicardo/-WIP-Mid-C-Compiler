#include "prim_type.h"
#include "backend_dependent/type_sizes.h"
#include "structs.h"
#include <assert.h>

bool PrimitiveType_signed(enum PrimitiveType type, unsigned lvls_of_indir) {

    return lvls_of_indir == 0 && (
            type == PrimType_INT || type == PrimType_CHAR ||
            type == PrimType_SHORT || type == PrimType_LONG
            );

}

unsigned PrimitiveType_size(enum PrimitiveType type, unsigned lvls_of_indir,
        u32 type_idx, const struct StructList *structs) {

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

    case PrimType_STRUCT:
        return structs->elems[type_idx].size;

    default:
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

    case PrimType_STRUCT:
        return PrimType_STRUCT;

    default:
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

bool PrimitiveType_non_prim_type(enum PrimitiveType type) {

    return type == PrimType_STRUCT;

}

bool PrimitiveType_can_convert_to(enum PrimitiveType conv_dest,
        unsigned dest_indir, u32 dest_type_idx, enum PrimitiveType conv_src,
        unsigned src_indir, u32 src_type_idx) {

    if (dest_type_idx != src_type_idx)
        return false;

    /* any pointer can get cast to a void pointer */
    else if (dest_indir == 1 && conv_dest == PrimType_VOID && src_indir > 0)
        return true;
    /* and a void pointer can be cast to any other pointer */
    else if (src_indir == 1 && conv_src == PrimType_VOID && dest_indir > 0)
        return true;

    else if (conv_dest == conv_src && dest_indir > 0 == src_indir > 0)
        return true;
    else if (conv_dest == conv_src)
        return false;

    else if (PrimitiveType_non_prim_type(conv_dest) ==
            PrimitiveType_non_prim_type(conv_src) && dest_indir == src_indir)
        return true;

    return false;

}
