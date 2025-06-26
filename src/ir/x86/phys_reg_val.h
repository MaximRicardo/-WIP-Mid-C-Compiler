#pragma once

/* info about which virtual register a physical register is holding */
struct PhysRegVal {

    const char *virt_reg;

};

struct PhysRegVal PhysRegVal_init(void);
struct PhysRegVal PhysRegVal_create(const char *virt_reg);
