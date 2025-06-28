#include "virt_to_phys.h"
#include "reg_lifetimes.h"
#include "phys_reg_val.h"
#include "../../attrib.h"
#include "../../utils/dyn_str.h"
#include "../../utils/make_str_cpy.h"
#include "reg_states.h"
#include "remove_alloc_reg.h"
#include "../../macros.h"
#include "../ir_to_str.h"
#include <stdio.h>
#include <stdlib.h>
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

ATTRIBUTE((unused))
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

    if (strncmp(preg->virt_reg, "__", 2) == 0)
        return false;

    printf("preg virt reg = %%%s\n", preg->virt_reg);
    reg_lt_idx = IRRegLTList_find_reg(vreg_lts, preg->virt_reg);
    assert(reg_lt_idx != m_u32_max);
    if (vreg_lts->elems[reg_lt_idx].death_idx < instr_idx)
        return true;

    return false;

}

/* instr_idx is relative to the start of the function. returns the idx of
 * the preg holding vreg.
 * ret_if_no_free_regs              - i recommend leaving this false. in
 *                                    normal operation, there should never
 *                                    be a lack of registers */
static u32 alloc_vreg(const char *vreg, const struct IRBasicBlock *cur_block,
        const struct IRFunc *cur_func, u32 instr_idx,
        const struct IRRegLTList *vreg_lts, bool ret_if_no_free_regs) {

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
        if (ret_if_no_free_regs)
            return m_u32_max;
        else
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
            vreg_lts, false);

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

static char* create_stack_offset_str(u32 offset) {

    struct DynamicStr str = DynamicStr_init();

    DynamicStr_append_printf(&str, "__esp(%u)", offset);

    return str.str;

}

/* __esp(x) -> __esp(x+amount) */
static char* incr_stack_offset(const char *stack_offset, u32 amount) {

    u32 old_offset;
    struct DynamicStr str = DynamicStr_init();

    assert(strncmp(stack_offset, "__esp(", 6) == 0);

    old_offset = strtoul(&stack_offset[6], NULL, 0);

    DynamicStr_append_printf(&str, "__esp(%u)", old_offset+amount);

    return str.str;

}

/* increases all the stack ptr offsets by amount */
static void incr_func_stack_size(struct IRFunc *func, u32 amount) {

    u32 i;

    for (i = 0; i < func->vregs.size; i++) {

        char *new_vreg_name = NULL;

        if (strncmp(func->vregs.elems[i], "__esp(", 6) != 0)
            continue;

        new_vreg_name = incr_stack_offset(func->vregs.elems[i], amount);
        IRFunc_rename_vreg(func, func->vregs.elems[i], new_vreg_name);

    }

}

static u32 get_n_vregs_to_alloca_in_instr(struct IRInstr *instr, u32 instr_idx,
        const struct IRRegLTList *vreg_lts,
        const struct IRBasicBlock *cur_block, const struct IRFunc *cur_func) {

    u32 i;
    u32 n_vregs_to_alloca = 0;

    for (i = 0; i < instr->args.size; i++) {

        if (instr->args.elems[i].type != IRInstrArg_REG)
            continue;

        n_vregs_to_alloca +=
            alloc_vreg(instr->args.elems[i].value.reg_name, cur_block,
                    cur_func, instr_idx, vreg_lts, true) == m_u32_max;

    }

    return n_vregs_to_alloca;

}

/* DOESN'T PRESERVE THE CPU REGISTER STATES */
static u32 get_n_vregs_to_alloca_in_block(struct IRBasicBlock *block,
        u32 start_instr_idx, const struct IRRegLTList *vreg_lts,
        const struct IRFunc *cur_func) {

    u32 i;
    u32 n_vregs_to_alloca = 0;
    u32 instr_idx = start_instr_idx;

    for (i = 0; i < block->instrs.size; i++) {

        n_vregs_to_alloca += get_n_vregs_to_alloca_in_instr(
                &block->instrs.elems[i], instr_idx, vreg_lts, block, cur_func
                );

        ++instr_idx;

    }

    return n_vregs_to_alloca;

}

/* selects n vregs in block to put on the stack. */
static void alloca_vregs_in_block(struct IRBasicBlock *block, u32 n,
        struct IRFunc *parent, struct IRRegLTList *vreg_lts) {

    u32 i;
    struct ConstStringList vregs = IRBasicblock_get_vregs(block, true);

    u32 none_reg_idx = StringList_find(&parent->vregs, "__none");
    if (none_reg_idx == m_u32_max) {
        StringList_push_back(&parent->vregs, make_str_copy("__none"));
        none_reg_idx = parent->vregs.size-1;
    }

    n *= 2;

    assert(vregs.size >= n);

    printf("none = %p\n", (void*)parent->vregs.elems[n]);
    printf("n = %u\n", n);

    for (i = 0; i < parent->vregs.size; i++) {
        printf("func vreg %%%s at %p\n", parent->vregs.elems[i],
                (void*)parent->vregs.elems[i]);
    }

    /* make sure to properly allocate the new stack space */
    incr_func_stack_size(parent, n*4);

    /* for now just alloca the first n of them */
    for (i = 0; i < m_min(vregs.size, n); i++) {

        u32 j;
        u32 reg_lt_idx;

        /* start from the top of the newly allocated stack space */
        char *sp_offset = create_stack_offset_str((n-i - 1)*4);
        printf("i = %u, sp_offset = %p\n", i, (void*)sp_offset);

        /* we don't wanna allocate accidentally a register on the stack */
        if (strncmp(vregs.elems[i], "__", 2) == 0)
            continue;

        printf("sp offset = %%%s\n", sp_offset);
        for (j = i; j < vregs.size; j++) {
            fprintf(stderr, "vreg %%%s, %p\n", vregs.elems[j],
                    (void*)vregs.elems[j]);
        }

        IRInstrList_push_back(&block->instrs, IRInstr_create_alloca(
                    parent->vregs.elems[none_reg_idx],
                    IRDataType_create(false, 32, 1), 4, 4
                    ));

        for (j = 0; j < parent->vregs.size; j++) {
            printf("func vreg %%%s at %p\n", parent->vregs.elems[j],
                    (void*)parent->vregs.elems[j]);
        }

        /* the vreg entry has to be removed from the vreg lts list */
        reg_lt_idx = IRRegLTList_find_reg(vreg_lts, vregs.elems[i]);
        assert(reg_lt_idx != m_u32_max);
        IRRegLTList_erase(vreg_lts, reg_lt_idx, NULL);

        IRFunc_rename_vreg(parent, vregs.elems[i], sp_offset);

    }

    ConstStringList_free(&vregs);

}

/* instr_idx is relative to the start of the function */
static void virt_to_phys_block(struct IRBasicBlock *block,
        struct IRFunc *cur_func, struct IRRegLTList *vreg_lts,
        u32 *instr_idx) {

    u32 i;
    u32 n_vregs_to_alloca;

    RegStatesList_push_back(&block_reg_states, RegStates_init());
    init_cpu_reg_vals(block, cur_func, vreg_lts);

    n_vregs_to_alloca =
        get_n_vregs_to_alloca_in_block(block, *instr_idx, vreg_lts, cur_func);
    init_cpu_reg_vals(block, cur_func, vreg_lts);

    alloca_vregs_in_block(block, n_vregs_to_alloca, cur_func, vreg_lts);

    /*
    if (n_vregs_to_alloca > 0)
        *vreg_lts = IRRegLTList_get_func_lts(cur_func);
    */

    for (i = 0; i < block->instrs.size; i++) {
        virt_to_phys_instr(&block->instrs.elems[i], block, cur_func, vreg_lts,
                *instr_idx);
        ++*instr_idx;
    }

}

static void virt_to_phys_func(struct IRFunc *func, struct IRModule *parent) {

    u32 i;

    struct IRRegLTList vreg_lts = IRRegLTList_get_func_lts(func);
    u32 instr_idx = 0;

    for (i = 0; i < func->blocks.size; i++) {
        virt_to_phys_block(&func->blocks.elems[i], func, &vreg_lts,
                &instr_idx);
        /*
        printf("after block %s\n", func->blocks.elems[i].label);
        printf("ir =\n%s\n", IRToStr_gen(parent));
        */
    }

    RegStatesList_free(&block_reg_states);

    IRRegLTList_free(&vreg_lts);

}

void X86_virt_to_phys(struct IRModule *module) {

    u32 i;

    for (i = 0; i < module->funcs.size; i++) {
        virt_to_phys_func(&module->funcs.elems[i], module);
    }

    X86_remove_alloc_reg(module);

}
