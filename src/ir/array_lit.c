#include "array_lit.h"
#include "../array_lit.h"
#include "../front_end/ast.h"

struct IRArrayLit IRArrayLit_init(void) {

    struct IRArrayLit lit;
    lit.name = NULL;
    lit.array = U32List_init();
    return lit;

}

struct IRArrayLit IRArrayLit_create(char *name, struct U32List array) {

    struct IRArrayLit lit;
    lit.name = name;
    lit.array = array;
    return lit;

}

void IRArrayLit_free(struct IRArrayLit lit) {

    m_free(lit.name);
    U32List_free(&lit.array);

}

void IRArrayLit_append_elems(struct IRArrayLit *self,
        const struct ArrayLit *lit) {

    u32 i;

    for (i = 0; i < lit->n_values; i++) {
        U32List_push_back(&self->array, lit->values[i]->int_value);
    }

}

m_define_VectorImpl_funcs(IRArrayLitList, struct IRArrayLit)
