#pragma once

#include "../../utils/vector_impl.h"

/* info about which virtual register a physical register is holding */
struct PhysRegVal {

    const char *virt_reg;

};

struct PhysRegVal PhysRegVal_init(void);
struct PhysRegVal PhysRegVal_create(const char *virt_reg);

struct PhysRegValList {

    struct PhysRegVal *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(PhysRegValList, struct PhysRegVal)
