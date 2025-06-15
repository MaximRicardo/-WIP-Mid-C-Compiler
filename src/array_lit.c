#include "array_lit.h"
#include "ast.h"
#include "safe_mem.h"
#include "vector_impl.h"
#include <stddef.h>

struct ArrayLit ArrayLit_init(void) {

    struct ArrayLit lit;
    lit.values = NULL;
    lit.n_values = 0;
    return lit;

}

struct ArrayLit ArrayLit_create(struct Expr **values, u32 n_values) {

    struct ArrayLit lit;
    lit.values = values;
    lit.n_values = n_values;
    return lit;

}

void ArrayLit_free(struct ArrayLit *self) {

    u32 i;
    for (i = 0; i < self->n_values; i++) {
        Expr_recur_free_w_self(self->values[i]);
    }

    m_free(self->values);

}

m_define_VectorImpl_funcs(ArrayLitList, struct ArrayLit)
