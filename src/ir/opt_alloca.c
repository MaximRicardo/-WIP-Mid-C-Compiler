#include "opt_alloca.h"
#include "basic_block.h"
#include "instr.h"
#include "../utils/make_str_cpy.h"
#include "../utils/dyn_str.h"
#include <stddef.h>
#include <assert.h>
#include <string.h>

/* information about a virtual register to convert from an alloca ptr to a
 * regular virtual reg */
struct RegToConv {

    char *old_name;
    char *new_name;
    bool initialized;

    u32 n_names_generated;

};

struct RegToConv RegToConv_init(void) {

    struct RegToConv x;
    x.old_name = NULL;
    x.new_name = NULL;
    x.initialized = false;
    x.n_names_generated = 0;
    return x;

}

struct RegToConv RegToConv_create(char *old_name, char *new_name,
        bool initialized) {

    struct RegToConv x;
    x.old_name = old_name;
    x.new_name = new_name;
    x.initialized = initialized;
    x.n_names_generated = 0;
    return x;

}

void RegToConv_free(struct RegToConv x) {

    m_free(x.old_name);
    m_free(x.new_name);

}

void RegToConv_gen_new_name(struct RegToConv *self) {

    struct DynamicStr new_new_name = DynamicStr_init();

    DynamicStr_append_printf(&new_new_name, "%s.%u", self->old_name,
            self->n_names_generated++);

    m_free(self->new_name);
    self->new_name = new_new_name.str;

}

struct RegToConvList {

    struct RegToConv *elems;
    u32 size;
    u32 capacity;

};

m_define_VectorImpl_funcs(RegToConvList, struct RegToConv)

u32 RegToConvList_find_reg(const struct RegToConvList *self,
        const char *old_name) {

    u32 i;

    for (i = 0; i < self->size; i++) {
        if (strcmp(self->elems[i].old_name, old_name) == 0)
            return i;
    }

    return m_u32_max;

}

static void create_new_reg_to_conv_from_alloca(const struct IRInstr *instr,
        struct RegToConvList *regs_to_conv) {

    /* dunno how to deal with stuff bigger than 4 bytes yet */
    if (instr->args.elems[1].value.imm_u32 > 4)
        assert(false);

    /* something like:
     *    alloca i32* %x, 4, 4
     *    store i32 5, i32* %x, 0
     * would get converted to:
     *    mov %x, 5
     * the register name would stay the same for now, hence why old_name and
     * new_name get set to the same string.
     */
    RegToConvList_push_back(regs_to_conv, RegToConv_create(
                make_str_copy(instr->args.elems[0].value.reg_name),
                make_str_copy(instr->args.elems[0].value.reg_name),
                false
                ));


}

/*
 * new_dest_name            - if not NULL, changes the name of the store
 *                            destination to this.
 */
static void store_to_mov_instr(struct IRInstr *instr,
        const char *new_dest_name) {

    struct IRInstr new_instr = IRInstr_init();
    new_instr.type = IRInstr_MOV;

    /* order gets flipped */
    IRInstrArgList_push_back(&new_instr.args, instr->args.elems[1]);
    IRInstrArgList_push_back(&new_instr.args, instr->args.elems[0]);

    /* the destination is no longer a pointer */
    assert(new_instr.args.elems[0].data_type.lvls_of_indir > 0);
    --new_instr.args.elems[0].data_type.lvls_of_indir;

    if (new_dest_name) {
        m_free(new_instr.args.elems[0].value.reg_name);
        new_instr.args.elems[0].value.reg_name = make_str_copy(new_dest_name);
    }

    m_free(instr->args.elems);
    *instr = new_instr;

}

/*
 * new_src_name            - if not NULL, changes the name of the store
 *                            destination to this.
 */
static void load_to_mov_instr(struct IRInstr *instr,
        const char *new_src_name) {

    struct IRInstr new_instr = IRInstr_init();
    new_instr.type = IRInstr_MOV;

    /* order stays the same */
    IRInstrArgList_push_back(&new_instr.args, instr->args.elems[0]);
    IRInstrArgList_push_back(&new_instr.args, instr->args.elems[1]);

    /* the source is no longer a pointer */
    assert(new_instr.args.elems[1].data_type.lvls_of_indir > 0);
    --new_instr.args.elems[1].data_type.lvls_of_indir;

    if (new_src_name) {
        m_free(new_instr.args.elems[1].value.reg_name);
        new_instr.args.elems[1].value.reg_name = make_str_copy(new_src_name);
    }

    m_free(instr->args.elems);
    *instr = new_instr;

}

static void convert_store_to_init(struct IRInstr *instr,
        struct RegToConvList *regs_to_conv) {

    const char *store_dest = instr->args.elems[1].value.reg_name;
    u32 r2c_idx;

    if (instr->args.elems[1].type != IRInstrArg_REG ||
            instr->args.elems[2].type != IRInstrArg_IMM32 ||
            instr->args.elems[2].value.imm_u32 != 0)
        return;

    r2c_idx = RegToConvList_find_reg(regs_to_conv, store_dest);

    assert(r2c_idx != m_u32_max);

    if (regs_to_conv->elems[r2c_idx].initialized) {
        RegToConv_gen_new_name(&regs_to_conv->elems[r2c_idx]);
    }
    else {
        regs_to_conv->elems[r2c_idx].initialized = true;
    }

    store_to_mov_instr(instr, regs_to_conv->elems[r2c_idx].new_name);

}

static void convert_load_to_mov(struct IRInstr *instr,
        const struct RegToConvList *regs_to_conv) {

    const char *load_src = instr->args.elems[1].value.reg_name;
    u32 r2c_idx;

    if (instr->args.elems[1].type != IRInstrArg_REG ||
            instr->args.elems[2].type != IRInstrArg_IMM32 ||
            instr->args.elems[2].value.imm_u32 != 0)
        return;

    r2c_idx = RegToConvList_find_reg(regs_to_conv, load_src);

    assert(r2c_idx != m_u32_max);

    load_to_mov_instr(instr, regs_to_conv->elems[r2c_idx].new_name);

}

static void opt_alloca_instr(struct IRInstr *instr,
        struct IRBasicBlock *cur_block, struct RegToConvList *regs_to_conv) {

    if (instr->type == IRInstr_ALLOCA) {
        create_new_reg_to_conv_from_alloca(instr, regs_to_conv);
        IRInstrList_erase(
                &cur_block->instrs, instr - cur_block->instrs.elems,
                IRInstr_free
                );
    }
    else if (instr->type == IRInstr_STORE) {
        convert_store_to_init(instr, regs_to_conv);
    }
    else if (instr->type == IRInstr_LOAD) {
        convert_load_to_mov(instr, regs_to_conv);
    }

}

static void opt_alloca_block(struct IRBasicBlock *block,
        struct RegToConvList *regs_to_conv) {

    u32 i;

    for (i = 0; i < block->instrs.size; i++) {
        opt_alloca_instr(&block->instrs.elems[i], block, regs_to_conv);
    }

}

static void opt_alloca_func(struct IRFunc *func) {

    u32 i;

    struct RegToConvList regs_to_conv = RegToConvList_init();

    for (i = 0; i < func->blocks.size; i++) {
        opt_alloca_block(&func->blocks.elems[i], &regs_to_conv);
    }

    while (regs_to_conv.size > 0) {
        RegToConvList_pop_back(&regs_to_conv, RegToConv_free);
    }
    RegToConvList_free(&regs_to_conv);

}

void IROpt_alloca(struct IRModule *module) {

    u32 i;

    for (i = 0; i < module->funcs.size; i++) {
        opt_alloca_func(&module->funcs.elems[i]);
    }

}
