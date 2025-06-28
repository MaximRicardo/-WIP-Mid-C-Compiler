#pragma once

#include "phys_reg_val.h"
#include "../reg_lifetimes.h"

/* not counting sp:
 * ax, bx, cx, dx, bp, si, di */
#define m_n_usable_pregs 7

struct RegStates {

    struct PhysRegVal preg_vals[m_n_usable_pregs];

};

struct RegStates RegStates_init(void);
void RegStates_merge(struct RegStates *self, const struct RegStates *other,
        const struct IRRegLTList *vreg_lts);

struct RegStatesList {

    struct RegStates *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(RegStatesList, struct RegStates)
