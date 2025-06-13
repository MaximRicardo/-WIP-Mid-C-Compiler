#include "parser_var.h"
#include <string.h>

struct ParserVar ParserVar_init(void) {

    struct ParserVar var;
    var.line_num = 0;
    var.column_num = 0;
    var.name = NULL;
    var.lvls_of_indir = 0;
    var.type = PrimType_INVALID;
    var.is_array = false;
    var.array_len = 0;
    var.stack_pos = 0;
    var.args = NULL;
    var.void_args = false;
    var.has_been_defined = false;
    return var;

}

struct ParserVar ParserVar_create(unsigned line_num, unsigned column_num,
        char *name, unsigned lvls_of_indir, enum PrimitiveType type,
        bool is_array, u32 array_len, u32 stack_pos,
        struct VarDeclPtrList *args, bool void_args, bool has_been_defined) {

    struct ParserVar var;
    var.line_num = line_num;
    var.column_num = column_num;
    var.name = name;
    var.lvls_of_indir = lvls_of_indir;
    var.type = type;
    var.is_array = is_array;
    var.array_len = array_len;
    var.stack_pos = stack_pos;
    var.args = args;
    var.void_args = void_args;
    var.has_been_defined = has_been_defined;
    return var;

}

void ParserVar_free(struct ParserVar var) {

    m_free(var.name);

}

u32 ParVarList_find_var(const struct ParVarList *self, char *name) {

    unsigned i;
    for (i = 0; i < self->size; i++) {
        if (strcmp(self->elems[i].name, name) == 0)
            return i;
    }

    return m_u32_max;

}

m_define_VectorImpl_funcs(ParVarList, struct ParserVar)
