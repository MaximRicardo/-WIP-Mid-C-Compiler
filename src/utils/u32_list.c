#include "u32_list.h"

m_define_VectorImpl_funcs(U32List, u32)

    u32 U32List_find(const struct U32List *self, u32 value)
{
    u32 i;

    for (i = 0; i < self->size; i++) {
        if (self->elems[i] == value)
            return i;
    }

    return m_u32_max;
}
