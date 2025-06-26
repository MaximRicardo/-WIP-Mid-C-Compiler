#include "func.h"
#include "basic_block.h"
#include "data_types.h"
#include "instr.h"
#include <stdio.h>
#include <string.h>

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
     * replacing vreg names, since they will point to the address we're about
     * to free. */
    m_free(self->vregs.elems[vreg_idx]);
    self->vregs.elems[vreg_idx] = new_name;

    return true;

}

m_define_VectorImpl_funcs(IRFuncArgList, struct IRFuncArg)
m_define_VectorImpl_funcs(IRFuncList, struct IRFunc)
