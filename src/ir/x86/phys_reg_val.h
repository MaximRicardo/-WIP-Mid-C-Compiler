#pragma once

/* info about which virtual register a physical register is holding */
struct PhysRegVal {

    char *virt_reg;

};

struct PhysRegVal PhysRegVal_init(void);
struct PhysRegVal PhysRegVal_create(char *virt_reg);
void PhysRegVal_free(struct PhysRegVal reg);
