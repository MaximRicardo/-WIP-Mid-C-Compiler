#include "parser_var.h"
#include "type_mods.h"
#include <string.h>

struct ParserVar ParserVar_init(void) {

    struct ParserVar var;
    var.line_num = 0;
    var.column_num = 0;
    var.name = NULL;
    var.lvls_of_indir = 0;
    var.mods = TypeModifiers_init();
    var.type = PrimType_INVALID;
    var.type_idx = 0;
    var.is_array = false;
    var.array_len = 0;
    var.stack_pos = 0;
    var.args = NULL;
    var.variadic_args = false;
    var.void_args = false;
    var.has_been_defined = false;
    var.is_func_arg = false;
    var.parent = NULL;
    return var;

}

struct ParserVar ParserVar_create(unsigned line_num, unsigned column_num,
        char *name, unsigned lvls_of_indir, struct TypeModifiers mods,
        enum PrimitiveType type, u32 type_idx,
        bool is_array, u32 array_len, u32 stack_pos,
        struct VarDeclPtrList *args, bool variadic_args, bool void_args,
        bool has_been_defined,
        bool is_func_arg, void *parent) {

    struct ParserVar var;
    var.line_num = line_num;
    var.column_num = column_num;
    var.name = name;
    var.lvls_of_indir = lvls_of_indir;
    var.mods = mods;
    var.type = type;
    var.type_idx = type_idx;
    var.is_array = is_array;
    var.array_len = array_len;
    var.stack_pos = stack_pos;
    var.args = args;
    var.variadic_args = variadic_args;
    var.void_args = void_args;
    var.has_been_defined = has_been_defined;
    var.is_func_arg = is_func_arg;
    var.parent = parent;
    return var;

}

void ParserVar_free(struct ParserVar var) {

    m_free(var.name);

}

u32 ParVarList_find_var(const struct ParVarList *self, char *name) {

    u32 i;

    /* counting down to start from the current scope. makes variable shadowing
     * work */
    for (i = self->size-1; i < self->size; i--) {
        if (strcmp(self->elems[i].name, name) == 0)
            return i;
    }

    return m_u32_max;

}

m_define_VectorImpl_funcs(ParVarList, struct ParserVar)
