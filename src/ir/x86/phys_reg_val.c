#include "phys_reg_val.h"
#include <stddef.h>

struct PhysRegVal PhysRegVal_init(void) {

    struct PhysRegVal reg;
    reg.virt_reg = NULL;
    return reg;

}

struct PhysRegVal PhysRegVal_create(const char *virt_reg) {

    struct PhysRegVal reg;
    reg.virt_reg = virt_reg;
    return reg;

}
