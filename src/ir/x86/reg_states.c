#include "reg_states.h"

struct RegStates RegStates_init(void) {

    u32 i;
    struct RegStates x;

    for (i = 0; i < m_n_usable_pregs; i++) {
        x.preg_vals[i] = PhysRegVal_init();
    }

    return x;

}

void RegStates_merge(struct RegStates *self, const struct RegStates *other,
        const struct IRRegLTList *vreg_lts) {

    u32 i;

    for (i = 0; i < m_n_usable_pregs; i++) {

        if (!self->preg_vals[i].virt_reg) {
            self->preg_vals[i].virt_reg = other->preg_vals[i].virt_reg;
        }
        else if (other->preg_vals[i].virt_reg) {
            u32 self_vreg = IRRegLTList_find_reg(
                    vreg_lts, self->preg_vals[i].virt_reg
                    );
            u32 other_vreg = IRRegLTList_find_reg(
                    vreg_lts, other->preg_vals[i].virt_reg
                    );

            if (vreg_lts->elems[other_vreg].death_idx >
                    vreg_lts->elems[self_vreg].death_idx) {
                self->preg_vals[i].virt_reg = other->preg_vals[i].virt_reg;
            }
        }

    }

}

struct RegStates RegStates_copy(const struct RegStates self) {

    /* works cur preg_vals is a regular array, meaning a regular copy
     * duplicates all of it's contents. */
    return self;

}

m_define_VectorImpl_funcs(RegStatesList, struct RegStates)
