#pragma once

#include "comp_dependent/ints.h"
#include "prim_type.h"
#include "vector_impl.h"

struct StructField {

    char *name;
    u32 offset;

    enum PrimitiveType type;
    unsigned lvls_of_indir;
    bool is_array;
    u32 array_len;

};

struct StructField StructField_init(void);
struct StructField StructField_create(char *name, u32 offset,
        enum PrimitiveType type, unsigned lvls_of_indir, bool is_array,
        u32 array_len);
void StructField_free(struct StructField field);

struct StructFieldList {

    struct StructField *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(StructFieldList, struct StructField)

struct Struct {

    char *name;

    struct StructFieldList members;

    bool defined;
    /* where the struct was first defined */
    unsigned def_line_num;
    const char *def_file_path;

    u32 size;
    u32 alignment;

};

struct Struct Struct_init(void);
struct Struct Struct_create(char *name, struct StructFieldList members,
        bool defined, unsigned def_line_num, const char *def_file_path,
        u32 size, u32 alignment);
void Struct_free(struct Struct x);
/* returns m_u32_max if a member with the given name couldn't be found */
u32 Struct_field_idx(struct Struct *x, const char *name);

struct StructList {

    struct Struct *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(StructList, struct Struct)

u32 StructList_find_struct(const struct StructList *self, const char *name);
