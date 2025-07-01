#pragma once

#include "../utils/bool.h"
#include "../prim_type.h"
#include "../comp_dependent/ints.h"
#include "../type_mods.h"
#include "../utils/vector_impl.h"

struct ParserVar {

    unsigned line_num, column_num;
    char *name;
    unsigned lvls_of_indir;
    struct TypeModifiers mods;
    enum PrimitiveType type;
    /* used as a struct idx for structs, and union idx for unions, etc.
     * should be kept at 0 for primitive types */
    u32 type_idx;
    bool is_array;
    u32 array_len;
    u32 stack_pos;

    /* used by functions */
    struct VarDeclPtrList *args;
    bool variadic_args;
    bool void_args; /* int func(void), for example. */
    bool has_been_defined;

    bool is_func_arg;
    /* points to a func node if the variable is a function argument, else
     * points to a block node */
    void *parent;

};

struct ParserVar ParserVar_init(void);
struct ParserVar ParserVar_create(unsigned line_num, unsigned column_num,
        char *name, unsigned lvls_of_indir, struct TypeModifiers mods,
        enum PrimitiveType type, u32 type_idx,
        bool is_array, u32 array_len, u32 stack_pos,
        struct VarDeclPtrList *args, bool variadic_args, bool void_args,
        bool has_been_defined, bool is_func_arg, void *parent);
void ParserVar_free(struct ParserVar var);

struct ParVarList {

    struct ParserVar *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(ParVarList, struct ParserVar)

/* returns m_u32_max if there is no var in the current scope by that name */
u32 ParVarList_find_var(const struct ParVarList *self, char *name);
