#pragma once

/* a function consists of several basic blocks */

#include "basic_block.h"
#include "data_types.h"
#include "../utils/string_list.h"

struct IRFuncArg {

    struct IRDataType type;
    char *name;

};

struct IRFuncArg IRFuncArg_init(void);
struct IRFuncArg IRFuncArg_create(struct IRDataType type, char *name);
void IRFuncArg_free(struct IRFuncArg arg);

struct IRFuncArgList {

    struct IRFuncArg *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(IRFuncArgList, struct IRFuncArg)

struct IRFunc {

    char *name;
    struct IRDataType ret_type;
    struct IRFuncArgList args;
    struct IRBasicBlockList blocks;
    /* the name of every virtual register in the function */
    struct StringList vregs;

};

struct IRFunc IRFunc_init(void);
struct IRFunc IRFunc_create(char *name, struct IRDataType ret_type,
        struct IRFuncArgList args, struct IRBasicBlockList blocks,
        struct StringList vregs);
void IRFunc_free(struct IRFunc func);
/* returns NULL if n is out of bounds */
struct IRInstr* IRFunc_get_nth_instr(const struct IRFunc *self, u32 n);
/* same as IRFunc_get_nth_instr except returns the block the instr is in */
struct IRBasicBlock* IRFunc_get_nth_instr_block(const struct IRFunc *self,
        u32 n);
/* returns false if there is no vreg in self named old_name, and returns true
 * if there is. */
bool IRFunc_rename_vreg(struct IRFunc *self, const char *old_name,
        char *new_name);
/* returns false if there is no vreg in self named name, and returns true if
 * there is. */
bool IRFunc_replace_vreg_w_instr_arg(struct IRFunc *self, const char *vreg,
        const struct IRInstrArg *arg);
u32 IRFunc_get_stack_size(const struct IRFunc *self);
u32 IRFunc_find_none_reg(const struct IRFunc *self);
bool IRFunc_vreg_in_phi_node(const struct IRFunc *self, const char *vreg);
void IRFunc_move_allocas_to_top(struct IRFunc *self);
u32 IRFunc_find_arg(const struct IRFunc *self, const char *name);
/* old and new vreg are indices in self->vregs.
 * WARNING:
 *    REMOVES OLD_VREG FROM SELF->VREGS. ANY INDICES GREATER THAN OLD_VREG MUST
 *    BE DECREMENTED! */
void IRFunc_replace_vreg(struct IRFunc *self, u32 old_vreg, u32 new_vreg);
bool IRFunc_has_body(const struct IRFunc *self);

struct IRFuncList {

    struct IRFunc *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(IRFuncList, struct IRFunc)
