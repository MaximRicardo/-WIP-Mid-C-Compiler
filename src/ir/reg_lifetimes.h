#pragma once

#include "../comp_dependent/ints.h"
#include "../utils/vector_impl.h"
#include "func.h"

/* LT is short for lifetime */
struct IRRegLT {

    const char *reg_name;
    /* the index of the instr (relative to the start of the function the instr
     * is in) where the virtual register was first created */
    u32 creation_idx;
    /* the index of the instr where it was last used */
    u32 death_idx;

};

struct IRRegLT IRRegLT_init(void);
struct IRRegLT IRRegLT_create(const char *reg_name, u32 creation_idx,
        u32 death_idx);

/* i'll turn this into a hashmap as soon as i get around to implementing
 * them */
struct IRRegLTList {

    struct IRRegLT *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(IRRegLTList, struct IRRegLT)

/* returns the index of the lifetime, or m_u32_max if it couldn't be found */
u32 IRRegLTList_find_reg(const struct IRRegLTList *self, const char *reg_name);

/* returns the lifetime of every virtual register in the function */
struct IRRegLTList IRRegLTList_get_func_lts(const struct IRFunc *func);
