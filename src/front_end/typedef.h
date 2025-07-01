#pragma once

#include "../comp_dependent/ints.h"
#include "../prim_type.h"
#include "../type_mods.h"
#include "../utils/vector_impl.h"

struct Typedef {

    char *type_name;
    enum PrimitiveType conv_type;
    unsigned conv_lvls_of_indir;
    struct TypeModifiers conv_mods;

};

struct Typedef Typedef_init(void);
struct Typedef Typedef_create(char *type_name, enum PrimitiveType conv_type,
        unsigned conv_lvls_of_indir, struct TypeModifiers conv_mods);
void Typedef_free(struct Typedef x);

struct TypedefList {

    struct Typedef *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(TypedefList, struct Typedef)
