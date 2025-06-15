#pragma once

#include "comp_dependent/ints.h"
#include "vector_impl.h"

/* TODO:
 *    make array literals fill uninitialized elements with zero.
 */

struct Expr;

struct ArrayLit {

    struct Expr **values;
    u32 n_values;

};

struct ArrayLit ArrayLit_init(void);
struct ArrayLit ArrayLit_create(struct Expr **values, u32 n_values);
void ArrayLit_free(struct ArrayLit *self);

struct ArrayLitList {

    struct ArrayLit *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(ArrayLitList, struct ArrayLit)
