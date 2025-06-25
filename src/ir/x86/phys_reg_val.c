#include "phys_reg_val.h"
#include "../../safe_mem.h"
#include <stddef.h>

struct PhysRegVal PhysRegVal_init(void) {

    struct PhysRegVal reg;
    reg.virt_reg = NULL;
    return reg;

}

struct PhysRegVal PhysRegVal_create(char *virt_reg) {

    struct PhysRegVal reg;
    reg.virt_reg = virt_reg;
    return reg;

}

void PhysRegVal_free(struct PhysRegVal reg) {

    m_free(reg.virt_reg);

}
