#include "reg_states.h"
#include "phys_reg_val.h"

struct RegStates RegStates_init(void) {

    u32 i;
    struct RegStates x;

    for (i = 0; i < m_n_usable_pregs; i++) {
        x.preg_vals[i] = PhysRegVal_init();
    }

    return x;

}

static const char* reg_states_merge_vregs(const char *self_vreg,
        const char *other_vreg, const struct IRRegLTList *vreg_lts) {

    if (!self_vreg) {
        return other_vreg;
    }
    else if (other_vreg) {
        u32 self_vreg_i = IRRegLTList_find_reg(
                vreg_lts, self_vreg
                );
        u32 other_vreg_i = IRRegLTList_find_reg(
                vreg_lts, other_vreg 
                );

        if (vreg_lts->elems[other_vreg_i].death_idx >
                vreg_lts->elems[self_vreg_i].death_idx) {
            return self_vreg;
        }
    }

    return self_vreg;

}

void RegStates_merge(struct RegStates *self, const struct RegStates *other,
        const struct IRRegLTList *vreg_lts) {

    u32 i;

    for (i = 0; i < m_n_usable_pregs; i++) {

        self->preg_vals[i].virt_reg = reg_states_merge_vregs(
                self->preg_vals[i].virt_reg, other->preg_vals[i].virt_reg,
                vreg_lts
                );

    }

}

m_define_VectorImpl_funcs(RegStatesList, struct RegStates)
