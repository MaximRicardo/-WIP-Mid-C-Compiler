#pragma once

#include "../comp_dependent/ints.h"
#include "vector_impl.h"

struct U32List {

    u32 *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(U32List, u32)

/* returns the idx, not the value. cuz that would be stupid */
u32 U32List_find(const struct U32List *self, u32 value);
