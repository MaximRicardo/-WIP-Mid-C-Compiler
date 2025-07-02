#include "func.h"
#include "basic_block.h"
#include "data_types.h"
#include "instr.h"
#include <string.h>
#include <assert.h>

static u32 round_up(u32 num, u32 multiple) {

    u32 remainder;

    if (multiple == 0)
        return num;

    remainder = num % multiple;
    if (remainder == 0)
        return num;

    return num+multiple-remainder;

}

struct IRFuncArg IRFuncArg_init(void) {

    struct IRFuncArg arg;
    arg.type = IRDataType_init();
    arg.name = NULL;
    return arg;

}

struct IRFuncArg IRFuncArg_create(struct IRDataType type, char *name) {

    struct IRFuncArg arg;
    arg.type = type;
    arg.name = name;
    return arg;

}

void IRFuncArg_free(struct IRFuncArg arg) {

    m_free(arg.name);

}

struct IRFunc IRFunc_init(void) {

    struct IRFunc func;
    func.name = NULL;
    func.ret_type = IRDataType_init();
    func.args = IRFuncArgList_init();
    func.blocks = IRBasicBlockList_init();
    func.vregs = StringList_init();
    return func;

}

struct IRFunc IRFunc_create(char *name, struct IRDataType ret_type,
        struct IRFuncArgList args, struct IRBasicBlockList blocks,
        struct StringList vregs) {

    struct IRFunc func;
    func.name = name;
    func.ret_type = ret_type;
    func.args = args;
    func.blocks = blocks;
    func.vregs = vregs;
    return func;

}

void IRFunc_free(struct IRFunc func) {

    m_free(func.name);

    while (func.blocks.size > 0) {
        IRBasicBlockList_pop_back(&func.blocks, IRBasicBlock_free);
    }
    IRBasicBlockList_free(&func.blocks);

    while (func.args.size > 0) {
        IRFuncArgList_pop_back(&func.args, IRFuncArg_free);
    }
    IRFuncArgList_free(&func.args);

    while (func.vregs.size > 0) {
        StringList_pop_back(&func.vregs, String_free);
    }
    StringList_free(&func.vregs);

}

struct IRInstr* IRFunc_get_nth_instr(const struct IRFunc *self, u32 n) {

    u32 i;
    u32 instr = 0;

    for (i = 0; i < self->blocks.size; i++) {

        if (n >= instr+self->blocks.elems[i].instrs.size) {
            instr += self->blocks.elems[i].instrs.size;
            continue;
        }

        return &self->blocks.elems[i].instrs.elems[n-instr];

    }

    return NULL;

}

struct IRBasicBlock* IRFunc_get_nth_instr_block(const struct IRFunc *self,
        u32 n) {

    u32 i;
    u32 instr = 0;

    for (i = 0; i < self->blocks.size; i++) {

        if (n >= instr+self->blocks.elems[i].instrs.size) {
            instr += self->blocks.elems[i].instrs.size;
            continue;
        }

        return &self->blocks.elems[i];

    }

    return NULL;

}

static void instr_replace_vreg_names(struct IRInstr *instr,
        const char *old_name, const char *new_name) {

    u32 i;

    for (i = 0; i < instr->args.size; i++) {

        if (instr->args.elems[i].type != IRInstrArg_REG)
            continue;

        if (strcmp(instr->args.elems[i].value.reg_name, old_name) == 0) {
            instr->args.elems[i].value.reg_name = new_name;
        }

    }

}

static void block_replace_vreg_names(struct IRBasicBlock *block,
        const char *old_name, const char *new_name) {

    u32 i;

    for (i = 0; i < block->instrs.size; i++) {
        instr_replace_vreg_names(&block->instrs.elems[i], old_name, new_name);
    }

}

bool IRFunc_rename_vreg(struct IRFunc *self, const char *old_name,
        char *new_name) {

    u32 i;

    u32 vreg_idx = m_u32_max;

    for (i = 0; i < self->vregs.size; i++) {

        if (strcmp(old_name, self->vregs.elems[i]) != 0)
            continue;

        vreg_idx = i;
        break;

    }

    if (vreg_idx == m_u32_max)
        return false;

    for (i = 0; i < self->blocks.size; i++) {
        block_replace_vreg_names(&self->blocks.elems[i], old_name, new_name);
    }

    /* free and replace at the end to prevent heap use after free when
     * replacing vreg names, since they will point to the address we're
     * about to free. */
    m_free(self->vregs.elems[vreg_idx]);
    self->vregs.elems[vreg_idx] = new_name;

    return true;

}

static void instr_replace_vreg_w_instr_arg(struct IRInstr *instr,
        const char *vreg, const struct IRInstrArg *new_arg) {

    u32 i;

    for (i = 0; i < instr->args.size; i++) {

        if (instr->args.elems[i].type != IRInstrArg_REG)
            continue;

        if (strcmp(instr->args.elems[i].value.reg_name, vreg) != 0)
            continue;

        instr->args.elems[i] = *new_arg;

    }

}

static void block_replace_vreg_w_instr_arg(struct IRBasicBlock *block,
        const char *vreg, const struct IRInstrArg *arg) {

    u32 i;

    for (i = 0; i < block->instrs.size; i++) {
        instr_replace_vreg_w_instr_arg(&block->instrs.elems[i], vreg, arg);
    }

}

bool IRFunc_replace_vreg_w_instr_arg(struct IRFunc *self, const char *vreg,
        const struct IRInstrArg *arg) {

    u32 i;
    u32 vreg_idx;

    for (i = 0; i < self->blocks.size; i++) {
        block_replace_vreg_w_instr_arg(&self->blocks.elems[i], vreg, arg);
    }

    vreg_idx = StringList_find(&self->vregs, vreg);
    if (vreg_idx != m_u32_max) {
        StringList_erase(&self->vregs, vreg_idx, String_free);
        return true;
    }
    else {
        return false;
    }

}

static void get_func_stack_size_block(const struct IRBasicBlock *block,
        u32 *stack_size) {

    u32 i;

    for (i = 0; i < block->instrs.size; i++) {

        struct IRInstr *instr = &block->instrs.elems[i];

        if (instr->type != IRInstr_ALLOCA)
            continue;

        *stack_size =
            round_up(*stack_size, instr->args.elems[Arg_RHS].value.imm_u32);

        *stack_size += instr->args.elems[Arg_LHS].value.imm_u32;

    }

}

u32 IRFunc_get_stack_size(const struct IRFunc *func) {

    u32 i;
    u32 stack_size = 0;

    for (i = 0; i < func->blocks.size; i++) {

        get_func_stack_size_block(&func->blocks.elems[i], &stack_size);

    }

    return stack_size;

}

u32 IRFunc_find_none_reg(const struct IRFunc *self) {

    const char *name = "__none";

    return StringList_find(&self->vregs, name);

}

static bool instr_vreg_in_phi_node(const struct IRInstr *instr,
        const char *vreg) {

    u32 i;

    if (instr->type != IRInstr_PHI)
        return false;

    for (i = 0; i < instr->args.size; i++) {

        struct IRInstrArg *arg = &instr->args.elems[i];

        assert(arg->type == IRInstrArg_REG);

        if (strcmp(arg->value.reg_name, vreg) != 0)
            continue;

        return true;

    }

    return false;

}

static bool block_vreg_in_phi_node(const struct IRBasicBlock *block,
        const char *vreg) {

    u32 i;

    for (i = 0; i < block->instrs.size; i++) {
        if (instr_vreg_in_phi_node(&block->instrs.elems[i], vreg))
            return true;
    }

    return false;

}

bool IRFunc_vreg_in_phi_node(const struct IRFunc *self, const char *vreg) {

    u32 i;

    for (i = 0; i < self->blocks.size; i++) {
        if (block_vreg_in_phi_node(&self->blocks.elems[i], vreg))
            return true;
    }

    return false;

}

static void move_allocas_to_top_block(struct IRBasicBlock *block,
        struct IRFunc *parent) {

    u32 i;

    for (i = 0; i < block->instrs.size; i++) {

        struct IRInstr *instr = &block->instrs.elems[i];
        struct IRBasicBlock *top = &parent->blocks.elems[0];

        if (instr->type != IRInstr_ALLOCA)
            continue;

        IRInstrList_push_front(&top->instrs, *instr);
        /* if the instr got inserted into the current block, we gotta account
         * for the instr indices being offset by one */
        i += top == block;

        IRInstrList_erase(&block->instrs, i, NULL);
        --i;

    }

}

void IRFunc_move_allocas_to_top(struct IRFunc *self) {

    u32 i;

    for (i = 0; i < self->blocks.size; i++) {
        move_allocas_to_top_block(&self->blocks.elems[i], self);
    }

}

u32 IRFunc_find_arg(const struct IRFunc *self, const char *name) {


    u32 i;

    for (i = 0; i < self->args.size; i++) {
        if (strcmp(self->args.elems[i].name, name) == 0)
            return i;
    }

    return m_u32_max;

}

void IRFunc_replace_vreg(struct IRFunc *self, u32 old_vreg, u32 new_vreg) {

    const char *old = self->vregs.elems[old_vreg];
    char *new = self->vregs.elems[new_vreg];

    assert(old_vreg != new_vreg);

    assert(IRFunc_rename_vreg(self, old, new));

    StringList_erase(&self->vregs, old_vreg, NULL);

}

m_define_VectorImpl_funcs(IRFuncArgList, struct IRFuncArg)
m_define_VectorImpl_funcs(IRFuncList, struct IRFunc)
