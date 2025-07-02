#include "data_types.h"
#include "../backend_dependent/type_sizes.h"
#include <assert.h>

struct IRDataType IRDataType_init(void) {

    struct IRDataType x;
    x.is_signed = false;
    x.width = 0;
    x.lvls_of_indir = 0;
    return x;

}

struct IRDataType IRDataType_create(bool is_signed, u32 width,
        u32 lvls_of_indir) {

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
    /* the lvls of indir is already accounted for by x.lvls_of_indir, so it
     * should be set to zero here to get the width of the actual base type */
    if (type != PrimType_VOID)
        x.width = PrimitiveType_size(type, 0, type_idx, structs)*8;
    else
        x.width = 0;
    return x;

}

enum PrimitiveType IRDataType_to_prim_type(const struct IRDataType *self) {

    if (self->width == m_TypeSize_char*8)
        return self->is_signed ? PrimType_CHAR : PrimType_UCHAR;
    else if (self->width == m_TypeSize_short*8)
        return self->is_signed ? PrimType_SHORT : PrimType_USHORT;
    else if (self->width == m_TypeSize_int*8)
        return self->is_signed ? PrimType_INT : PrimType_UINT;
    else if (self->width == m_TypeSize_long*8)
        return self->is_signed ? PrimType_LONG : PrimType_ULONG;
    else
        assert(false);

}

u32 IRDataType_real_width(const struct IRDataType *self) {

    return self->lvls_of_indir == 0 ? self->width : 32;

}

u32 IRDataType_deref_real_width(const struct IRDataType *self) {

    assert(self->lvls_of_indir > 0);

    return self->lvls_of_indir-1 == 0 ? self->width : 32;

}
