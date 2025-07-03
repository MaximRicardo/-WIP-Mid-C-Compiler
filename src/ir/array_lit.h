#pragma once

#include "../utils/u32_list.h"
#include "../utils/vector_impl.h"
#include "../array_lit.h"

struct IRArrayLit {

    char *name;
    struct U32List array;
    u32 elem_width;

};

struct IRArrayLit IRArrayLit_init(void);
struct IRArrayLit IRArrayLit_create(char *name, struct U32List array,
        u32 elem_width);
void IRArrayLit_free(struct IRArrayLit lit);
void IRArrayLit_append_elems(struct IRArrayLit *self,
        const struct ArrayLit *lit);

struct IRArrayLitList {

    struct IRArrayLit *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(IRArrayLitList, struct IRArrayLit)
