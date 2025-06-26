#pragma once

#include "../comp_dependent/ints.h"
#include "../vector_impl.h"

/* apparently you can't do StringList_pop_back(&list, free), because for some
 * reason char*'s can't be cast to void*'s anymore? */
void String_free(char *str);

struct StringList {

    char **elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(StringList, char*)

u32 StringList_find(const struct StringList *self, const char *str);

struct ConstStringList {

    const char **elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(ConstStringList, const char*)

u32 ConstStringList_find(const struct ConstStringList *self, const char *str);
