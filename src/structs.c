#include "structs.h"
#include "comp_dependent/ints.h"
#include "prim_type.h"
#include "safe_mem.h"
#include "vector_impl.h"
#include <string.h>

struct StructField StructField_init(void) {

    struct StructField field;
    field.name = NULL;
    field.offset = 0;
    field.type = PrimType_INVALID;
    field.lvls_of_indir = 0;
    field.is_array = false;
    field.array_len = 0;
    return field;

}

struct StructField StructField_create(char *name, u32 offset,
        enum PrimitiveType type, unsigned lvls_of_indir, bool is_array,
        u32 array_len) {

    struct StructField field;
    field.name = name;
    field.offset = offset;
    field.type = type;
    field.lvls_of_indir = lvls_of_indir;
    field.is_array = is_array;
    field.array_len = array_len;
    return field;

}

void StructField_free(struct StructField field) {

    m_free(field.name);

}

struct Struct Struct_init(void) {

    struct Struct x;
    x.name = NULL;
    x.members = StructFieldList_init();
    x.defined = false;
    x.alignment = 0;
    return x;

}

struct Struct Struct_create(char *name, struct StructFieldList members,
        bool defined, u32 alignment) {

    struct Struct x;
    x.name = name;
    x.members = members;
    x.defined = defined;
    x.alignment = alignment;
    return x;

}

void Struct_free(struct Struct x) {

    m_free(x.name);

    while (x.members.size > 0)
        StructFieldList_pop_back(&x.members, StructField_free);
    StructFieldList_free(&x.members);

}

u32 Struct_field_idx(struct Struct *x, const char *name) {

    u32 i;

    for (i = 0; i < x->members.size; i++) {
        if (strcmp(name, x->members.elems[i].name) == 0)
            return i;
    }

    return m_u32_max;

}

m_define_VectorImpl_funcs(StructFieldList, struct StructField)
m_define_VectorImpl_funcs(StructList, struct Struct)
