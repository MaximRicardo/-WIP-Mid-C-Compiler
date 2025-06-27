#include "virt_to_phys.h"
#include "reg_lifetimes.h"
#include "phys_reg_val.h"
#include "../../attrib.h"
#include "../../utils/dyn_str.h"
#include "reg_states.h"
#include "remove_alloc_reg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

enum CPURegs {

    CPUReg_AX,
    CPUReg_BX,
    CPUReg_CX,
    CPUReg_DX,
    CPUReg_BP,
    CPUReg_SI,
    CPUReg_DI

};

/* must be in the same order as enum CPURegs. */
static const char *cpu_regs[] = {

    "eax",
    "ebx",
    "ecx",
    "edx",
    "ebp",
    "esi",
    "edi"

};

/* the reg states for every block in the current IRFunc that has been processed
 * thus far */
static struct RegStatesList block_reg_states;


static struct RegStates* RegStates_block_preg_states(
        const struct IRBasicBlock *block, const struct IRFunc *func) {

    return &block_reg_states.elems[block - func->blocks.elems];

}

ATTRIBUTE((unused))
static void print_cpu_reg_vals(const struct IRBasicBlock *cur_block,
        const struct IRFunc *cur_func) {

    u32 i;

    struct RegStates *cpu_reg_vals =
        RegStates_block_preg_states(cur_block, cur_func);

    for (i = 0; i < m_n_usable_pregs; i++) {
        printf("%s: %%%s\n", cpu_regs[i], cpu_reg_vals->preg_vals[i].virt_reg);
    }

}

static void init_cpu_reg_vals(const struct IRBasicBlock *cur_block,
        const struct IRFunc *cur_func, const struct IRRegLTList *vreg_lts) {

    u32 i;

    struct RegStates *cpu_reg_vals =
        RegStates_block_preg_states(cur_block, cur_func);

    for (i = 0; i < m_n_usable_pregs; i++) {
        cpu_reg_vals->preg_vals[i] = PhysRegVal_init();
    }

    for (i = 0; i < cur_block->imm_doms.size; i++) {
        const struct IRBasicBlock *other =
            &cur_func->blocks.elems[cur_block->imm_doms.elems[i]];

        RegStates_merge(cpu_reg_vals,
                RegStates_block_preg_states(other, cur_func),
                vreg_lts
                );
    }

    /*
    printf("init regs result on block %lu\n",
            cur_block-cur_func->blocks.elems);
    print_cpu_reg_vals(cur_block, cur_func);*/

}

static bool is_cpu_reg_free(u32 reg, const struct IRBasicBlock *cur_block,
        const struct IRFunc *cur_func) {

    struct RegStates *cpu_reg_vals =
        RegStates_block_preg_states(cur_block, cur_func);

    return !cpu_reg_vals->preg_vals[reg].virt_reg;

}

/* returns m_u32_max if there is no preg holding vreg */
static u32 preg_holding_vreg(const char *vreg,
        const struct IRBasicBlock *cur_block, const struct IRFunc *cur_func) {

    u32 i;

    struct RegStates *reg_states =
        RegStates_block_preg_states(cur_block, cur_func);

    for (i = 0; i < m_n_usable_pregs; i++) {
        if (!reg_states->preg_vals[i].virt_reg)
            continue;

        if (strcmp(reg_states->preg_vals[i].virt_reg, vreg) == 0)
            return i;
    }

    return m_u32_max;

}

/* instr_idx is relative to the start of the function */
static bool preg_is_free(u32 preg_idx,
        const struct IRBasicBlock *cur_block, const struct IRFunc *cur_func,
        u32 instr_idx, const struct IRRegLTList *vreg_lts) {

    const struct RegStates *reg_states =
        RegStates_block_preg_states(cur_block, cur_func);

    const struct PhysRegVal *preg = &reg_states->preg_vals[preg_idx];
    u32 reg_lt_idx;

    if (!preg->virt_reg)
        return true;

    reg_lt_idx = IRRegLTList_find_reg(vreg_lts, preg->virt_reg);
    if (vreg_lts->elems[reg_lt_idx].death_idx < instr_idx)
        return true;

    return false;

}

/* instr_idx is relative to the start of the function. returns the idx of
 * the preg holding vreg */
static u32 alloc_vreg(const char *vreg, const struct IRBasicBlock *cur_block,
        const struct IRFunc *cur_func, u32 instr_idx,
        const struct IRRegLTList *vreg_lts) {

    u32 i;
    u32 alloc_dest = m_u32_max;

    struct RegStates *reg_states =
        RegStates_block_preg_states(cur_block, cur_func);

    if (preg_holding_vreg(vreg, cur_block, cur_func) != m_u32_max) {
        return preg_holding_vreg(vreg, cur_block, cur_func);
    }

    for (i = 0; i < m_n_usable_pregs; i++) {

        if (preg_is_free(i, cur_block, cur_func, instr_idx, vreg_lts)) {
            alloc_dest = i;
            break;
        }

    }

    if (alloc_dest == m_u32_max) {
        fprintf(stderr, "RAN OUT OF CPU REGISTERS\n");
        assert(false);
    }

    reg_states->preg_vals[alloc_dest].virt_reg = vreg;
    return alloc_dest;

}

/* instr_idx is relative to the start of the function */
static void virt_to_phys_instr_arg(struct IRInstrArg *arg,
        struct IRBasicBlock *cur_block, struct IRFunc *cur_func,
        const struct IRRegLTList *vreg_lts, u32 instr_idx) {

    u32 phys_reg;

    struct DynamicStr new_vreg_name;

    if (arg->type != IRInstrArg_REG)
        return;

    if (strncmp(arg->value.reg_name, "__", 2) == 0)
        return;

    phys_reg = alloc_vreg(arg->value.reg_name, cur_block, cur_func, instr_idx,
            vreg_lts);

    new_vreg_name = DynamicStr_init();
    /* the reg names are prefixed with '__' to ensure there won't be any
     * clashing with other vreg names */
    DynamicStr_append_printf(&new_vreg_name, "__%s", cpu_regs[phys_reg]);

    StringList_push_back(&cur_func->vregs, new_vreg_name.str);
    arg->value.reg_name = new_vreg_name.str;

}

/* instr_idx is relative to the start of the function */
static void virt_to_phys_instr(struct IRInstr *instr,
        struct IRBasicBlock *cur_block, struct IRFunc *cur_func,
        const struct IRRegLTList *vreg_lts, u32 instr_idx) {

    u32 i;

    for (i = 0; i < instr->args.size; i++) {

        virt_to_phys_instr_arg(&instr->args.elems[i], cur_block, cur_func,
                vreg_lts, instr_idx);

    }

}

/* instr_idx is relative to the start of the function */
static void virt_to_phys_block(struct IRBasicBlock *block,
        struct IRFunc *cur_func, const struct IRRegLTList *vreg_lts,
        u32 *instr_idx) {

    u32 i;

    RegStatesList_push_back(&block_reg_states, RegStates_init());
    init_cpu_reg_vals(block, cur_func, vreg_lts);

    for (i = 0; i < block->instrs.size; i++) {
        virt_to_phys_instr(&block->instrs.elems[i], block, cur_func, vreg_lts,
                *instr_idx);
        ++*instr_idx;
    }

}

static void virt_to_phys_func(struct IRFunc *func) {

    u32 i;

    struct IRRegLTList vreg_lts = IRRegLTList_get_func_lts(func);
    u32 instr_idx = 0;

    for (i = 0; i < func->blocks.size; i++) {
        virt_to_phys_block(&func->blocks.elems[i], func, &vreg_lts,
                &instr_idx);
    }

    RegStatesList_free(&block_reg_states);

    IRRegLTList_free(&vreg_lts);

}

void X86_virt_to_phys(struct IRModule *module) {

    u32 i;

    printf("virt to phys\n");

    for (i = 0; i < module->funcs.size; i++) {
        virt_to_phys_func(&module->funcs.elems[i]);
    }

    X86_remove_alloc_reg(module);

}
