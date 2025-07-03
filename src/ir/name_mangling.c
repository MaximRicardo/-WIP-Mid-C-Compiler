#include "name_mangling.h"
#include "../utils/dyn_str.h"
#include <string.h>

char* IR_demangle(const char *name) {

    u32 i = 0;

    struct DynamicStr str = DynamicStr_init();

    while (name[i] != '\0' && name[i] != '.') {
        DynamicStr_append_char(&str, name[i]);
        ++i;
    }

    return str.str;

}

struct IRNameMangle IRNameMangle_init(void) {

    struct IRNameMangle x;
    x.og_name = NULL;
    x.new_names = ConstStringList_init();
    return x;

}

struct IRNameMangle IRNameMangle_create(char *og_name,
        struct ConstStringList new_names) {

    struct IRNameMangle x;
    x.og_name = og_name;
    x.new_names = new_names;
    return x;

}

void IRNameMangle_free(struct IRNameMangle x) {

    m_free(x.og_name);
    ConstStringList_free(&x.new_names);

}

m_define_VectorImpl_funcs(IRNameMangleList, struct IRNameMangle)

u32 IRNameMangleList_find(const struct IRNameMangleList *self,
        const char *og_name) {

    u32 i;

    for (i = 0; i < self->size; i++) {
        if (strcmp(self->elems[i].og_name, og_name) == 0)
            return i;
    }

    return m_u32_max;

}
