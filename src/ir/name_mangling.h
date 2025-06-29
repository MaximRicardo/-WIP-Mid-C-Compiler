#pragma once

#include "../comp_dependent/ints.h"
#include "../utils/string_list.h"
#include "../vector_impl.h"

struct IRNameMangle {

    char *og_name;
    /* acts as a stack of new names. as variables of the same name get
     * declared, the currently active mangled name will be at the back of
     * new_names. */
    struct ConstStringList new_names;

};

struct IRNameMangle IRNameMangle_init(void);
struct IRNameMangle IRNameMangle_create(char *og_name,
        struct ConstStringList new_names);
void IRNameMangle_free(struct IRNameMangle x);

struct IRNameMangleList {

    struct IRNameMangle *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(IRNameMangleList, struct IRNameMangle)

u32 IRNameMangleList_find(const struct IRNameMangleList *self,
        const char *og_name);
