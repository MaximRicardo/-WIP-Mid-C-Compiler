#include "virt_to_phys.h"
#include "reg_lifetimes.h"
#include "phys_reg_val.h"
#include "../../attrib.h"
#include "../../utils/dyn_str.h"
#include "../../utils/make_str_cpy.h"
#include "reg_states.h"
#include "remove_alloc_reg.h"
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

/* keeps track of spilled registers */
static struct PhysRegValList reg_stack;

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

/* returns m_u32_max if vreg is not stored on the register stack */
static u32 reg_stack_holding_vreg(const char *vreg) {

    u32 i;

    for (i = 0; i < reg_stack.size; i++) {
        if (!reg_stack.elems[i].virt_reg)
            return i;

        if (strcmp(reg_stack.elems[i].virt_reg, vreg) == 0)
            return i;
    }

    return m_u32_max;

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

static bool phys_reg_val_is_free(const struct PhysRegVal *preg,
        u32 instr_idx, const struct IRRegLTList *vreg_lts) {

    u32 reg_lt_idx;

    if (!preg->virt_reg)
        return true;

    /* reserved vregs are permanent */
    if (strncmp(preg->virt_reg, "__", 2) == 0)
        return false;

    reg_lt_idx = IRRegLTList_find_reg(vreg_lts, preg->virt_reg);
    assert(reg_lt_idx != m_u32_max);
    if (vreg_lts->elems[reg_lt_idx].death_idx < instr_idx) {
        printf("%%%s dies at %u %u\n", vreg_lts->elems[reg_lt_idx].reg_name,
                vreg_lts->elems[reg_lt_idx].death_idx, instr_idx);
        return true;
    }

    return false;

}

/* instr_idx is relative to the start of the function */
static bool preg_is_free(u32 preg_idx,
        const struct IRBasicBlock *cur_block, const struct IRFunc *cur_func,
        u32 instr_idx, const struct IRRegLTList *vreg_lts) {

    const struct RegStates *reg_states =
        RegStates_block_preg_states(cur_block, cur_func);

    const struct PhysRegVal *preg = &reg_states->preg_vals[preg_idx];
    return phys_reg_val_is_free(preg, instr_idx, vreg_lts);

}

/* instr_idx is relative to the start of the function. returns the idx of
 * the preg holding vreg, or the idx of the elem in the reg stack holding vreg.
 * if the returned idx is an index into the reg stack, *idx_in_reg_stack will
 * be set to true. */
static u32 alloc_vreg(const char *vreg, struct IRBasicBlock *cur_block,
        struct IRFunc *cur_func, u32 instr_idx,
        const struct IRRegLTList *vreg_lts, bool *idx_in_reg_stack) {

    u32 i;
    u32 alloc_dest = m_u32_max;

    struct RegStates *reg_states =
        RegStates_block_preg_states(cur_block, cur_func);

    if (idx_in_reg_stack)
        *idx_in_reg_stack = false;

    i = preg_holding_vreg(vreg, cur_block, cur_func);
    if (i != m_u32_max) {
        return i;
    }

    i = reg_stack_holding_vreg(vreg);
    if (i != m_u32_max) {
        if (idx_in_reg_stack)
            *idx_in_reg_stack = true;

        return i;
    }

    for (i = 0; i < m_n_usable_pregs; i++) {
        if (preg_is_free(i, cur_block, cur_func, instr_idx, vreg_lts)) {
            alloc_dest = i;
            break;
        }
    }

    if (alloc_dest != m_u32_max) {
        reg_states->preg_vals[alloc_dest].virt_reg = vreg;
        return alloc_dest;
    }

    /* past this point, any alloc dest would be in the stack. */
    if (idx_in_reg_stack)
        *idx_in_reg_stack = true;

    for (i = 0; i < reg_stack.size; i++) {
        if (phys_reg_val_is_free(&reg_stack.elems[i], instr_idx, vreg_lts)) {
            alloc_dest = i;
            break;
        }
    }

    if (alloc_dest != m_u32_max) {
        reg_stack.elems[alloc_dest].virt_reg = vreg;
        return alloc_dest;
    }

    /* if the vreg can't be allocated anywhere, it has to be spilt onto
     * the stack */
    PhysRegValList_push_back(&reg_stack, PhysRegVal_create(vreg));

    /* add a new alloca instr to store the fact that the func's stack size has
     * been increased */
    {
        u32 none_idx = IRFunc_find_none_reg(cur_func);
        if (none_idx == m_u32_max) {
            StringList_push_back(&cur_func->vregs, make_str_copy("__none"));
            none_idx = cur_func->vregs.size-1;
        }

        IRInstrList_push_back(
                &IRBasicBlockList_back_ptr(&cur_func->blocks)->instrs,
                IRInstr_create_alloca(
                    cur_func->vregs.elems[none_idx],
                    IRDataType_create(false, 0, 1), 4, 4
                ));
    }

    printf("reg_stack_size = %u, allocated %%%s\n", reg_stack.size, vreg);
    return reg_stack.size-1;

}

/* instr_idx is relative to the start of the function */
static void virt_to_phys_instr_arg(struct IRInstrArg *arg,
        struct IRBasicBlock *cur_block, struct IRFunc *cur_func,
        const struct IRRegLTList *vreg_lts, u32 instr_idx) {

    u32 reg_idx;
    bool on_reg_stack;

    struct DynamicStr new_vreg_name;

    if (arg->type != IRInstrArg_REG)
        return;

    if (strncmp(arg->value.reg_name, "__", 2) == 0)
        return;

    reg_idx = alloc_vreg(arg->value.reg_name, cur_block, cur_func, instr_idx,
            vreg_lts, &on_reg_stack);

    new_vreg_name = DynamicStr_init();

    /* the reg names are prefixed with '__' to ensure there won't be any
     * clashing with other vreg names */
    if (on_reg_stack) {
        /* it's important that __new_esp is used instead of __esp, cuz
         * later we'll need to add an offset to every esp offset from before
         * this pass, and this makes it possible to distinguish the old ones
         * from the new ones. */
        DynamicStr_append_printf(&new_vreg_name, "__new_esp(%u)", reg_idx*4);
    }
    else {
        DynamicStr_append_printf(&new_vreg_name, "__%s", cpu_regs[reg_idx]);
    }

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
        struct IRFunc *cur_func, struct IRRegLTList *vreg_lts,
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

    u32 start_last_block_n_instrs =
        IRBasicBlockList_back_ptr(&func->blocks)->instrs.size;

    struct IRRegLTList vreg_lts = IRRegLTList_get_func_lts(func);
    u32 instr_idx = 0;

    reg_stack = PhysRegValList_init();

    for (i = 0; i < func->blocks.size; i++) {
        virt_to_phys_block(&func->blocks.elems[i], func, &vreg_lts,
                &instr_idx);
    }

    /* each appended alloca instruction will be 4 bytes long, and require a 4
     * byte alignment which esp already has. */
    incr_func_stack_size(func,
            (IRBasicBlockList_back_ptr(&func->blocks)->instrs.size -
                start_last_block_n_instrs)*4);

    for (i = 0; i < func->vregs.size; i++) {
        struct DynamicStr str;

        if (strncmp(func->vregs.elems[i], "__new_esp", 9) != 0)
            continue;

        str = DynamicStr_init();
        DynamicStr_append_printf(&str, "__esp%s", &func->vregs.elems[i][9]);

        IRFunc_rename_vreg(func, func->vregs.elems[i], str.str);
    }

    PhysRegValList_free(&reg_stack);
    RegStatesList_free(&block_reg_states);

    IRRegLTList_free(&vreg_lts);

}

void X86_virt_to_phys(struct IRModule *module) {

    u32 i;

    for (i = 0; i < module->funcs.size; i++) {
        virt_to_phys_func(&module->funcs.elems[i]);
    }

    X86_remove_alloc_reg(module);

}
