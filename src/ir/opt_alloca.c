#include "opt_alloca.h"
#include "../utils/dyn_str.h"
#include "../utils/make_str_cpy.h"
#include "basic_block.h"
#include "data_types.h"
#include "instr.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

struct StoreInfo {
    const char *old_name;
    const char *new_name;
    const struct IRBasicBlock *parent;
};

static struct StoreInfo StoreInfo_create(const char *old_name,
                                         const char *new_name,
                                         const struct IRBasicBlock *parent)
{
    struct StoreInfo info;
    info.old_name = old_name;
    info.new_name = new_name;
    info.parent = parent;
    return info;
}

struct StoreInfoList {
    struct StoreInfo *elems;
    u32 size;
    u32 capacity;
};

m_define_VectorImpl_funcs(StoreInfoList, struct StoreInfo)

    u32 StoreInfoList_find_block(const struct StoreInfoList *self,
                                 const struct IRBasicBlock *block)
{
    u32 i;

    for (i = 0; i < self->size; i++) {
        if (self->elems[i].parent == block)
            return i;
    }

    return m_u32_max;
}

/* information about a virtual register to convert from an alloca ptr to a
 * regular virtual reg */
struct RegToConv {
    const char *old_name;
    const char *new_name;
    bool initialized;

    struct IRDataType d_type;

    /* info about the last store instruction converted to a mov instr in each
     * block */
    struct StoreInfoList last_store;

    u32 n_names_generated;
};

static void StoreInfo_update(struct StoreInfo *self,
                             const struct RegToConv *r2c)
{
    self->new_name = r2c->new_name;
}

static struct RegToConv RegToConv_create(const char *old_name,
                                         const char *new_name, bool initialized,
                                         struct IRDataType d_type)
{
    struct RegToConv x;
    x.old_name = old_name;
    x.new_name = new_name;
    x.initialized = initialized;
    x.d_type = d_type;
    x.last_store = StoreInfoList_init();
    x.n_names_generated = 0;
    return x;
}

void RegToConv_free(struct RegToConv x)
{
    StoreInfoList_free(&x.last_store);
}

void RegToConv_gen_new_name(struct RegToConv *self, struct IRFunc *parent)
{
    struct DynamicStr new_new_name = DynamicStr_init();

    DynamicStr_append_printf(&new_new_name, "%s.%u", self->old_name,
                             self->n_names_generated++);

    StringList_push_back(&parent->vregs, new_new_name.str);
    self->new_name = new_new_name.str;
}

struct RegToConvList {
    struct RegToConv *elems;
    u32 size;
    u32 capacity;
};

m_define_VectorImpl_funcs(RegToConvList, struct RegToConv)

    u32 RegToConvList_find_reg(const struct RegToConvList *self,
                               const char *old_name)
{
    u32 i;

    for (i = 0; i < self->size; i++) {
        if (strcmp(self->elems[i].old_name, old_name) == 0) {
            return i;
        }
    }

    return m_u32_max;
}

static bool alloca_required_instr(const char *reg, const struct IRInstr *instr)
{
    u32 i;

    if (IRInstrType_is_mem_instr(instr->type))
        return false;

    for (i = 0; i < instr->args.size; i++) {
        if (instr->args.elems[i].type != IRInstrArg_REG)
            continue;

        if (strcmp(instr->args.elems[i].value.reg_name, reg) == 0)
            return true;
    }

    return false;
}

static bool alloca_required_block(const char *reg,
                                  const struct IRBasicBlock *block)
{
    u32 i;

    for (i = 0; i < block->instrs.size; i++) {
        if (alloca_required_instr(reg, &block->instrs.elems[i]))
            return true;
    }

    return false;
}

/* searches through the given function to determine whether or not it's
 * necessary that reg stays alloca'd. assumes reg is a ptr to the result of an
 * alloca instruction */
static bool alloca_required(const char *reg, const struct IRFunc *func)
{
    u32 i;

    for (i = 0; i < func->blocks.size; i++) {
        if (alloca_required_block(reg, &func->blocks.elems[i]))
            return true;
    }

    return false;
}

static void
create_new_reg_to_conv_from_alloca(const struct IRInstr *instr,
                                   struct RegToConvList *regs_to_conv)
{
    /* anything bigger than 4 bytes can't be held in a virtual register.
     * for now just crash if anything bigger than 4 bytes is alloca'd. */
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
    RegToConvList_push_back(
        regs_to_conv,
        RegToConv_create(
            instr->args.elems[0].value.reg_name,
            instr->args.elems[0].value.reg_name, false,
            IRDataType_create(instr->args.elems[0].data_type.is_signed,
                              instr->args.elems[0].data_type.width,
                              instr->args.elems[0].data_type.lvls_of_indir -
                                  1)));
}

/*
 * new_dest_name            - if not NULL, changes the name of the store
 *                            destination to this.
 */
static void store_to_mov_instr(struct IRInstr *instr, const char *new_dest_name)
{
    struct IRInstr new_instr = IRInstr_init();
    new_instr.type = IRInstr_MOV;

    /* order gets flipped */
    IRInstrArgList_push_back(&new_instr.args, instr->args.elems[1]);
    IRInstrArgList_push_back(&new_instr.args, instr->args.elems[0]);

    /* the destination is no longer a pointer */
    assert(new_instr.args.elems[0].data_type.lvls_of_indir > 0);
    --new_instr.args.elems[0].data_type.lvls_of_indir;

    if (new_dest_name) {
        new_instr.args.elems[0].value.reg_name = new_dest_name;
    }

    m_free(instr->args.elems);
    *instr = new_instr;
}

/*
 * new_src_name            - if not NULL, changes the name of the store
 *                            destination to this.
 */
static void load_to_mov_instr(struct IRInstr *instr, const char *new_src_name)
{
    struct IRInstr new_instr = IRInstr_init();
    new_instr.type = IRInstr_MOV;

    /* order stays the same */
    IRInstrArgList_push_back(&new_instr.args, instr->args.elems[0]);
    IRInstrArgList_push_back(&new_instr.args, instr->args.elems[1]);

    /* the source is no longer a pointer */
    assert(new_instr.args.elems[1].data_type.lvls_of_indir > 0);
    --new_instr.args.elems[1].data_type.lvls_of_indir;

    if (new_src_name) {
        new_instr.args.elems[1].value.reg_name = new_src_name;
    }

    m_free(instr->args.elems);
    *instr = new_instr;
}

static void convert_store_to_init(struct IRInstr *instr,
                                  struct IRBasicBlock *cur_block,
                                  struct IRFunc *cur_func,
                                  struct RegToConvList *regs_to_conv)
{
    const char *store_dest = instr->args.elems[1].value.reg_name;
    u32 r2c_idx;
    u32 block_str_i;

    if (instr->args.elems[1].type != IRInstrArg_REG ||
        instr->args.elems[2].type != IRInstrArg_IMM32 ||
        instr->args.elems[2].value.imm_u32 != 0)
        return;

    r2c_idx = RegToConvList_find_reg(regs_to_conv, store_dest);

    /* if it's not in the list, it's not to be converted to a virtual reg */
    if (r2c_idx == m_u32_max) {
        return;
    }

    if (regs_to_conv->elems[r2c_idx].initialized) {
        RegToConv_gen_new_name(&regs_to_conv->elems[r2c_idx], cur_func);
    } else {
        regs_to_conv->elems[r2c_idx].initialized = true;
    }

    block_str_i = StoreInfoList_find_block(
        &regs_to_conv->elems[r2c_idx].last_store, cur_block);

    if (block_str_i != m_u32_max) {
        StoreInfo_update(
            &regs_to_conv->elems[r2c_idx].last_store.elems[block_str_i],
            &regs_to_conv->elems[r2c_idx]);
    } else {
        block_str_i = regs_to_conv->elems[r2c_idx].last_store.size;
        StoreInfoList_push_back(
            &regs_to_conv->elems[r2c_idx].last_store,
            StoreInfo_create(regs_to_conv->elems[r2c_idx].old_name,
                             regs_to_conv->elems[r2c_idx].new_name, cur_block));
    }

    store_to_mov_instr(instr, regs_to_conv->elems[r2c_idx].new_name);
}

static void convert_load_to_mov(struct IRInstr *instr,
                                const struct RegToConvList *regs_to_conv)
{
    const char *load_src = instr->args.elems[1].value.reg_name;
    u32 r2c_idx;

    if (instr->args.elems[1].type != IRInstrArg_REG ||
        instr->args.elems[2].type != IRInstrArg_IMM32 ||
        instr->args.elems[2].value.imm_u32 != 0)
        return;

    r2c_idx = RegToConvList_find_reg(regs_to_conv, load_src);

    /* if it's not in the list, it's not to be converted to a virtual reg */
    if (r2c_idx == m_u32_max)
        return;

    load_to_mov_instr(instr, regs_to_conv->elems[r2c_idx].new_name);
}

/* skips all of block's imm doms that are in ignore_list.
 * ignore_list CAN be NULL. */
static void add_imm_doms_to_list(const struct IRBasicBlock *block,
                                 struct U32List *list,
                                 const struct U32List *ignore_list)
{
    u32 i;

    for (i = 0; i < block->imm_doms.size; i++) {
        u32 new_block = block->imm_doms.elems[i];

        /* prevents infinite looping */
        if (ignore_list && U32List_find(ignore_list, new_block) != m_u32_max)
            continue;

        U32List_push_back(list, new_block);
    }
}

static void create_phi_node_args(u32 r2c_idx, struct IRInstr *instr,
                                 const struct IRBasicBlock *start_block,
                                 const struct IRFunc *func,
                                 const struct RegToConvList *regs_to_conv)
{
    u32 i;
    struct U32List block_stack = U32List_init();
    struct U32List done_blocks = U32List_init();

    for (i = 0; i < start_block->imm_doms.size; i++) {
        U32List_push_back(&block_stack, start_block->imm_doms.elems[i]);
    }

    while (block_stack.size) {
        u32 store_idx;
        const char *store_name;

        const struct IRBasicBlock *block =
            &func->blocks.elems[U32List_back(&block_stack)];
        U32List_push_back(&done_blocks, U32List_back(&block_stack));
        U32List_pop_back(&block_stack, NULL);

        store_idx = StoreInfoList_find_block(
            &regs_to_conv->elems[r2c_idx].last_store, block);

        if (store_idx == m_u32_max) {
            add_imm_doms_to_list(block, &block_stack, &done_blocks);
            continue;
        }

        store_name =
            regs_to_conv->elems[r2c_idx].last_store.elems[store_idx].new_name;

        IRInstrArgList_push_back(
            &instr->args,
            IRInstrArg_create(IRInstrArg_REG,
                              regs_to_conv->elems[r2c_idx].d_type,
                              IRInstrArgValue_reg_name(store_name)));
    }

    U32List_free(&done_blocks);
    U32List_free(&block_stack);
}

/* WARNING: WILL PROBABLY BREAK IF THE IMMEDIATE DOMINATOR IS BELOW BLOCK!
 * sets the current r2c new_name to it's first immediate dominator's last store
 * info's new name */
static void set_new_name_to_imm_dom_new_name(u32 r2c_idx,
                                             const struct IRBasicBlock *block,
                                             const struct IRFunc *cur_func,
                                             struct RegToConvList *regs_to_conv)
{
    const struct StoreInfoList *info = &regs_to_conv->elems[r2c_idx].last_store;

    const struct IRBasicBlock *imm_dom =
        &cur_func->blocks.elems[block->imm_doms.elems[0]];

    u32 idx = StoreInfoList_find_block(info, imm_dom);

    if (idx == m_u32_max)
        return;

    regs_to_conv->elems[r2c_idx].new_name = info->elems[idx].new_name;
}

/* generates a temporary phi node with no arguments. this tells the next pass
 * to convert these temp nodes into real phi nodes once all the alloca's have
 * been converted. the temp phi nodes are structured like so:
 *    phi [VREG_NAME], [R2C_IDX]
 */
static bool create_temp_phi_node(u32 r2c_idx, struct IRBasicBlock *block,
                                 struct IRFunc *func,
                                 struct RegToConvList *regs_to_conv)
{
    struct IRInstr instr;
    char *vreg = NULL;

    struct RegToConv *r2c = &regs_to_conv->elems[r2c_idx];

    /* no phi nodes needed if there isn't any converging program flow */
    if (block->imm_doms.size == 0) {
        return false;
    } else if (block->imm_doms.size == 1) {
        set_new_name_to_imm_dom_new_name(r2c_idx, block, func, regs_to_conv);
        return false;
    }

    instr = IRInstr_create(IRInstr_PHI, IRInstrArgList_init());

    RegToConv_gen_new_name(r2c, func);
    StoreInfoList_push_back(
        &r2c->last_store,
        StoreInfo_create(r2c->old_name, r2c->new_name, block));

    vreg = make_str_copy(regs_to_conv->elems[r2c_idx].new_name);

    StringList_push_back(&func->vregs, vreg);

    IRInstrArgList_push_back(
        &instr.args,
        IRInstrArg_create(IRInstrArg_REG, regs_to_conv->elems[r2c_idx].d_type,
                          IRInstrArgValue_reg_name(vreg)));

    IRInstrArgList_push_back(
        &instr.args, IRInstrArg_create(IRInstrArg_IMM32, IRDataType_init(),
                                       IRInstrArgValue_imm_u32(r2c_idx)));

    IRInstrList_push_front(&block->instrs, instr);

    return true;
}

/* returns true if the temp phi node got replaced with a real one, and false
 * if the phi node is actually unnecessary and needs to be erased.*/
static bool convert_temp_phi_to_real(struct IRInstr *temp_phi,
                                     struct IRBasicBlock *cur_block,
                                     struct IRFunc *cur_func,
                                     const struct RegToConvList *regs_to_conv)
{
    u32 r2c_idx = temp_phi->args.elems[Arg_LHS].value.imm_u32;
    struct IRInstr instr = IRInstr_create(IRInstr_PHI, IRInstrArgList_init());

    assert(temp_phi->args.size == 2);

    IRInstrArgList_push_back(&instr.args, temp_phi->args.elems[Arg_SELF]);

    create_phi_node_args(r2c_idx, &instr, cur_block, cur_func, regs_to_conv);

    /* if instr.args.size is 1, then there was only one instance of the vreg in
     * the dominating blocks, and the phi node is unnecessary. */
    if (instr.args.size > 2) {
        /* we gotta update the store info so that dependent blocks can chain
         * phi nodes together */

        u32 idx = StoreInfoList_find_block(
            &regs_to_conv->elems[r2c_idx].last_store, cur_block);

        if (idx == m_u32_max) {
            StoreInfoList_push_back(
                &regs_to_conv->elems[r2c_idx].last_store,
                StoreInfo_create(regs_to_conv->elems[r2c_idx].old_name,
                                 regs_to_conv->elems[r2c_idx].new_name,
                                 cur_block));
        }

        IRInstr_free(*temp_phi);
        *temp_phi = instr;
        return true;
    } else {
        IRInstr_free(instr);
        return false;
    }
}

/* returns whether or not instr got erased from cur_block->instrs */
static bool opt_alloca_instr(struct IRInstr *instr,
                             struct IRBasicBlock *cur_block,
                             struct IRFunc *cur_func,
                             struct RegToConvList *regs_to_conv)
{
    if (instr->type == IRInstr_ALLOCA &&
        !alloca_required(instr->args.elems[0].value.reg_name, cur_func)) {
        create_new_reg_to_conv_from_alloca(instr, regs_to_conv);
        IRInstrList_erase(&cur_block->instrs, instr - cur_block->instrs.elems,
                          IRInstr_free);
        return true;
    } else if (instr->type == IRInstr_STORE) {
        convert_store_to_init(instr, cur_block, cur_func, regs_to_conv);
        return false;
    } else if (instr->type == IRInstr_LOAD) {
        convert_load_to_mov(instr, regs_to_conv);
        return false;
    }

    return false;
}

static void opt_alloca_block(struct IRBasicBlock *block,
                             struct IRFunc *cur_func,
                             struct RegToConvList *regs_to_conv)
{
    u32 i;

    for (i = 0; i < regs_to_conv->size; i++) {
        if (!create_temp_phi_node(i, block, cur_func, regs_to_conv) &&
            block->imm_doms.size > 0) {
            /* make sure to use the same virt reg as this block's dominator(s)
             */
            const struct IRBasicBlock *dom =
                &cur_func->blocks.elems[block->imm_doms.elems[0]];

            u32 store_idx = StoreInfoList_find_block(
                &regs_to_conv->elems[i].last_store, dom);

            if (store_idx != m_u32_max) {
                regs_to_conv->elems[i].new_name =
                    regs_to_conv->elems[i].last_store.elems[store_idx].new_name;
            }
        }
    }

    for (i = 0; i < block->instrs.size; i++) {
        i -= opt_alloca_instr(&block->instrs.elems[i], block, cur_func,
                              regs_to_conv);
    }

    for (i = 0; i < regs_to_conv->size; i++) {
        u32 j =
            StoreInfoList_find_block(&regs_to_conv->elems[i].last_store, block);

        if (j == m_u32_max)
            continue;
    }
}

static void block_fill_phi_nodes(struct IRBasicBlock *block,
                                 struct IRFunc *parent,
                                 const struct RegToConvList *regs_to_conv)
{
    u32 i;

    for (i = 0; i < block->instrs.size; i++) {
        struct IRInstr *instr = &block->instrs.elems[i];

        if (instr->type != IRInstr_PHI)
            continue;

        if (!convert_temp_phi_to_real(instr, block, parent, regs_to_conv)) {
            IRInstrList_erase(&block->instrs, i, IRInstr_free);
            --i;
        }
    }
}

/* a sub-pass to replace the temporary, empty phi nodes with real ones */
static void func_fill_phi_nodes(struct IRFunc *func,
                                const struct RegToConvList *regs_to_conv)
{
    u32 i;

    for (i = 0; i < func->blocks.size; i++) {
        block_fill_phi_nodes(&func->blocks.elems[i], func, regs_to_conv);
    }
}

static void opt_alloca_func(struct IRFunc *func)
{
    u32 i;

    struct RegToConvList regs_to_conv = RegToConvList_init();

    for (i = 0; i < func->blocks.size; i++) {
        opt_alloca_block(&func->blocks.elems[i], func, &regs_to_conv);
    }

    func_fill_phi_nodes(func, &regs_to_conv);

    while (regs_to_conv.size > 0) {
        RegToConvList_pop_back(&regs_to_conv, RegToConv_free);
    }
    RegToConvList_free(&regs_to_conv);
}

void IROpt_alloca(struct IRModule *module)
{
    u32 i;

    for (i = 0; i < module->funcs.size; i++) {
        opt_alloca_func(&module->funcs.elems[i]);
    }
}
