#include "typedef.h"
#include "prim_type.h"
#include "safe_mem.h"
#include "vector_impl.h"
#include <stddef.h>

struct Typedef Typedef_init(void) {

    struct Typedef x;
    x.type_name = NULL;
    x.conv_type = PrimType_INVALID;
    x.conv_lvls_of_indir = 0;
    return x;

}

struct Typedef Typedef_create(char *type_name, enum PrimitiveType conv_type,
        unsigned conv_lvls_of_indir) {

    struct Typedef x;
    x.type_name = type_name;
    x.conv_type = conv_type;
    x.conv_lvls_of_indir = conv_lvls_of_indir;
    return x;

}

void Typedef_free(struct Typedef x) {

    m_free(x.type_name);

}

m_define_VectorImpl_funcs(TypedefList, struct Typedef)
