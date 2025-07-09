#include "string_list.h"
#include <string.h>

void String_free(char *str)
{
    m_free(str);
}

u32 StringList_find(const struct StringList *self, const char *str)
{
    u32 i;

    for (i = 0; i < self->size; i++) {
        if (strcmp(self->elems[i], str) == 0)
            return i;
    }

    return m_u32_max;
}

u32 ConstStringList_find(const struct ConstStringList *self, const char *str)
{
    u32 i;

    for (i = 0; i < self->size; i++) {
        if (strcmp(self->elems[i], str) == 0)
            return i;
    }

    return m_u32_max;
}

m_define_VectorImpl_funcs(StringList, char *)
    m_define_VectorImpl_funcs(ConstStringList, const char *)
