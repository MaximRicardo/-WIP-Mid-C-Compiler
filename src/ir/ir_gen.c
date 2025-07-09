#include "ir_gen.h"
#include "../utils/dyn_str.h"
#include "../utils/make_str_cpy.h"
#include "array_lit.h"
#include "basic_block.h"
#include "block_dom.h"
#include "data_types.h"
#include "func.h"
#include "instr.h"
#include "module.h"
#include "name_mangling.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* do not reset between func definitions */
static u32 array_lit_counter = 0;

/* reset between func definitions */
static u32 reg_counter = 0;
static u32 if_else_counter = 0;
static u32 while_loop_counter = 0;
static u32 for_loop_counter = 0;

struct IRNameMangleList name_mangles;

static void block_node_gen_ir(struct BlockNode *block, struct IRFunc *cur_func,
                              struct IRModule *module,
                              const struct TranslUnit *tu);

static char *create_new_reg(struct IRFunc *func)
{
    struct DynamicStr name = DynamicStr_init();

    DynamicStr_append_printf(&name, "%u", reg_counter++);

    StringList_push_back(&func->vregs, name.str);

    return name.str;
}

static char *create_string_with_num_appended(const char *str, u32 num)
{
    struct DynamicStr new_str = DynamicStr_init();

    DynamicStr_append_printf(&new_str, "%s%u", str, num);

    return new_str.str;
}

/* offset is in bytes */
static void store_gen_ir(const char *src_reg, const char *dest_reg, u32 offset,
                         struct IRDataType src_type,
                         struct IRBasicBlock *cur_block)
{
    struct IRInstr instr = IRInstr_create(IRInstr_STORE, IRInstrArgList_init());

    IRInstrArgList_push_back(
        &instr.args, IRInstrArg_create(IRInstrArg_REG, src_type,
                                       IRInstrArgValue_reg_name(src_reg)));

    IRInstrArgList_push_back(
        &instr.args,
        IRInstrArg_create(IRInstrArg_REG,
                          IRDataType_create(src_type.is_signed, src_type.width,
                                            src_type.lvls_of_indir + 1),
                          IRInstrArgValue_reg_name(dest_reg)));

    IRInstrArgList_push_back(
        &instr.args,
        IRInstrArg_create(IRInstrArg_IMM32, IRDataType_create(false, 32, 0),
                          IRInstrArgValue_imm_u32(offset)));

    IRInstrList_push_back(&cur_block->instrs, instr);
}

/* offset is in bytes */
static void store_imm_gen_ir(i32 imm, const char *dest_reg, u32 offset,
                             struct IRDataType src_type,
                             struct IRBasicBlock *cur_block)
{
    struct IRInstr instr = IRInstr_create(IRInstr_STORE, IRInstrArgList_init());

    IRInstrArgList_push_back(&instr.args,
                             IRInstrArg_create(IRInstrArg_IMM32, src_type,
                                               IRInstrArgValue_imm_u32(imm)));

    IRInstrArgList_push_back(
        &instr.args,
        IRInstrArg_create(IRInstrArg_REG,
                          IRDataType_create(src_type.is_signed, src_type.width,
                                            src_type.lvls_of_indir + 1),
                          IRInstrArgValue_reg_name(dest_reg)));

    IRInstrArgList_push_back(
        &instr.args,
        IRInstrArg_create(IRInstrArg_IMM32, IRDataType_create(false, 32, 0),
                          IRInstrArgValue_imm_u32(offset)));

    IRInstrList_push_back(&cur_block->instrs, instr);
}

/* offset is in bytes */
static void load_gen_ir(const char *dest_reg, const char *src_reg, u32 offset,
                        struct IRDataType dest_type,
                        struct IRBasicBlock *cur_block)
{
    struct IRInstr instr = IRInstr_create(IRInstr_LOAD, IRInstrArgList_init());

    IRInstrArgList_push_back(
        &instr.args, IRInstrArg_create(IRInstrArg_REG, dest_type,
                                       IRInstrArgValue_reg_name(dest_reg)));

    IRInstrArgList_push_back(
        &instr.args, IRInstrArg_create(
                         IRInstrArg_REG,
                         IRDataType_create(dest_type.is_signed, dest_type.width,
                                           dest_type.lvls_of_indir + 1),
                         IRInstrArgValue_reg_name(src_reg)));

    IRInstrArgList_push_back(
        &instr.args,
        IRInstrArg_create(IRInstrArg_IMM32, IRDataType_create(false, 32, 0),
                          IRInstrArgValue_imm_u32(offset)));

    IRInstrList_push_back(&cur_block->instrs, instr);
}

static const char *get_mangled_ident(const char *ident)
{
    u32 idx = IRNameMangleList_find(&name_mangles, ident);
    struct IRNameMangle *p = NULL;

    if (idx == m_u32_max)
        return NULL;

    p = &name_mangles.elems[idx];

    if (p->new_names.size == 0)
        return NULL;

    return ConstStringList_back(&p->new_names);
}

static const char *get_mangled_ident_expr(const struct Expr *expr)
{
    char *src = Expr_src(expr);

    const char *mangled_ver = get_mangled_ident(src);

    m_free(src);
    return mangled_ver;
}

/* returns the register holding the loaded value */
static const char *
load_ident_expr_gen_ir(const struct Expr *expr, bool load_reference,
                       const char *result_reg, struct IRBasicBlock *cur_block,
                       struct IRFunc *cur_func, const struct TranslUnit *tu)
{
    struct IRDataType d_type;
    const char *mangled_ident;
    const char *reg_name;

    d_type = IRDataType_create_from_prim_type(expr->prim_type, expr->type_idx,
                                              expr->lvls_of_indir, tu->structs);

    mangled_ident = get_mangled_ident_expr(expr);
    assert(mangled_ident);

    if (load_reference) {
        /* return a pointer to the variable instead */
        return mangled_ident;
    }

    if (result_reg) {
        reg_name = result_reg;
    } else {
        reg_name = create_new_reg(cur_func);
    }

    load_gen_ir(reg_name, mangled_ident, 0, d_type, cur_block);

    return reg_name;
}

static const char *
expr_gen_ir(const struct Expr *expr, const struct TranslUnit *tu,
            struct IRBasicBlock *cur_block, struct IRFunc *cur_func,
            struct IRModule *module, const char *result_reg,
            const struct IRDataType *result_reg_type, bool load_reference);

/* returns a list of vregs holding the arguments in left to right order */
static struct ConstStringList get_func_call_args(const struct Expr *expr,
                                                 const struct TranslUnit *tu,
                                                 struct IRBasicBlock *cur_block,
                                                 struct IRFunc *cur_func,
                                                 struct IRModule *module)
{
    u32 i;
    struct ConstStringList arg_vregs = ConstStringList_init();

    for (i = 0; i < expr->args.size; i++) {
        char *vreg = create_new_reg(cur_func);
        struct IRDataType d_type = IRDataType_create_from_prim_type(
            expr->prim_type, expr->type_idx, expr->lvls_of_indir, tu->structs);
        expr_gen_ir(expr->args.elems[i], tu, cur_block, cur_func, module, vreg,
                    &d_type, false);

        ConstStringList_push_back(&arg_vregs, vreg);
    }

    return arg_vregs;
}

static const char *get_func_call_name_ptr(const struct Expr *expr,
                                          const struct IRModule *module)
{
    char *name = Expr_src(expr);
    u32 idx = IRModule_find_func(module, name);

    m_free(name);
    return module->funcs.elems[idx].name;
}

/* returns the register the return value got put in */
static const char *
func_call_gen_ir(const struct Expr *expr, const struct TranslUnit *tu,
                 struct IRBasicBlock *cur_block, struct IRFunc *cur_func,
                 struct IRModule *module, const char *result_reg,
                 const struct IRDataType *result_reg_type)
{
    struct ConstStringList arg_vregs =
        get_func_call_args(expr, tu, cur_block, cur_func, module);

    const char *dest = result_reg ? result_reg : create_new_reg(cur_func);

    struct IRDataType d_type = result_reg_type
                                   ? *result_reg_type
                                   : IRDataType_create_from_prim_type(
                                         expr->prim_type, expr->type_idx,
                                         expr->lvls_of_indir, tu->structs);

    struct IRInstr instr =
        IRInstr_create_call(get_func_call_name_ptr(expr, module), dest, d_type,
                            &arg_vregs, expr, tu);

    IRInstrList_push_back(&cur_block->instrs, instr);

    ConstStringList_free(&arg_vregs);

    return dest;
}

static const char *array_lit_expr_gen_ir(
    const struct Expr *expr, struct IRBasicBlock *cur_block,
    struct IRFunc *cur_func, struct IRModule *module, const char *result_reg,
    const struct IRDataType *result_reg_type, const struct TranslUnit *tu)
{
    const char *dest = result_reg ? result_reg : create_new_reg(cur_func);
    struct IRDataType d_type = result_reg_type
                                   ? *result_reg_type
                                   : IRDataType_create_from_prim_type(
                                         expr->prim_type, expr->type_idx,
                                         expr->lvls_of_indir, tu->structs);
    const char *array = NULL;
    struct IRInstr instr;

    assert(array_lit_counter < module->array_lits.size);

    array = module->array_lits.elems[array_lit_counter++].name;

    instr = IRInstr_create_mov(
        dest, d_type,
        IRInstrArg_create(IRInstrArg_STR, d_type,
                          IRInstrArgValue_generic_str(array)));

    IRInstrList_push_back(&cur_block->instrs, instr);

    return dest;
}

/* returns the register the result of the expr got put into
 * bruh why did i make this function so long
 * result_reg               - if not NULL, the result of the expression will
 *                            get put in this register. if this isn't NULL,
 *                            result_reg_type must also not be NULL.
 * load_reference           - if true and if possible, a pointer to the result
 *                            of the expr will be loaded instead. */
static const char *
expr_gen_ir(const struct Expr *expr, const struct TranslUnit *tu,
            struct IRBasicBlock *cur_block, struct IRFunc *cur_func,
            struct IRModule *module, const char *result_reg,
            const struct IRDataType *result_reg_type, bool load_reference)
{
    struct IRInstr instr;

    const char *lhs_reg = NULL;
    const char *rhs_reg = NULL;
    const char *self_reg = NULL;

    if (expr->expr_type == ExprType_FUNC_CALL) {
        return func_call_gen_ir(expr, tu, cur_block, cur_func, module,
                                result_reg, result_reg_type);
    } else if (expr->expr_type == ExprType_INT_LIT) {
        if (result_reg) {
            struct IRDataType d_type = IRDataType_create_from_prim_type(
                expr->prim_type, expr->type_idx, expr->lvls_of_indir,
                tu->structs);
            instr = IRInstr_init();
            instr = IRInstr_create_mov(
                result_reg, *result_reg_type,
                IRInstrArg_create(IRInstrArg_IMM32, d_type,
                                  IRInstrArgValue_imm_u32(expr->int_value)));
            IRInstrList_push_back(&cur_block->instrs, instr);
            return result_reg;
        } else {
            return NULL;
        }
    } else if (expr->expr_type == ExprType_ARRAY_LIT) {
        return array_lit_expr_gen_ir(expr, cur_block, cur_func, module,
                                     result_reg, result_reg_type, tu);
    } else if (expr->expr_type == ExprType_IDENT) {
        return load_ident_expr_gen_ir(expr, load_reference, result_reg,
                                      cur_block, cur_func, tu);
    }
    instr = IRInstr_init();

    if (expr->lhs) {
        lhs_reg =
            expr_gen_ir(expr->lhs, tu, cur_block, cur_func, module, NULL, NULL,
                        expr->expr_type == ExprType_EQUAL ||
                            expr->expr_type == ExprType_REFERENCE);
    }

    if (expr->rhs) {
        rhs_reg = expr_gen_ir(expr->rhs, tu, cur_block, cur_func, module, NULL,
                              NULL, false);
    }

    if (!result_reg)
        self_reg = create_new_reg(cur_func);
    else
        self_reg = result_reg;

    /* the operands go in this order:
     * self, lhs, rhs
     */

    IRInstrArgList_push_back(
        &instr.args, IRInstrArg_create_from_expr(expr, tu->structs, self_reg));

    if (expr->lhs) {
        IRInstrArgList_push_back(
            &instr.args,
            IRInstrArg_create_from_expr(expr->lhs, tu->structs, lhs_reg));
    }

    if (expr->rhs) {
        IRInstrArgList_push_back(
            &instr.args,
            IRInstrArg_create_from_expr(expr->rhs, tu->structs, rhs_reg));
    }

    if (expr->expr_type == ExprType_PLUS) {
        instr.type = IRInstr_ADD;
    } else if (expr->expr_type == ExprType_MINUS) {
        instr.type = IRInstr_SUB;
    } else if (expr->expr_type == ExprType_MUL) {
        instr.type = IRInstr_MUL;
    } else if (expr->expr_type == ExprType_DIV) {
        instr.type = IRInstr_DIV;
    } else if (expr->expr_type == ExprType_EQUAL_TO) {
        instr.type = IRInstr_SET_EQ;
    } else if (expr->expr_type == ExprType_NOT_EQUAL_TO) {
        instr.type = IRInstr_SET_NEQ;
    } else if (expr->expr_type == ExprType_L_THAN) {
        instr.type = IRInstr_SET_LT;
    } else if (expr->expr_type == ExprType_L_THAN_OR_E) {
        instr.type = IRInstr_SET_LTEQ;
    } else if (expr->expr_type == ExprType_G_THAN) {
        instr.type = IRInstr_SET_GT;
    } else if (expr->expr_type == ExprType_G_THAN_OR_E) {
        instr.type = IRInstr_SET_GTEQ;
    } else if (expr->expr_type == ExprType_EQUAL) {
        struct IRDataType expr_d_type = IRDataType_create_from_prim_type(
            expr->prim_type, expr->type_idx, expr->lvls_of_indir, tu->structs);

        IRInstr_free(instr);

        if (rhs_reg) {
            store_gen_ir(rhs_reg, lhs_reg, 0, expr_d_type, cur_block);
        } else if (expr->rhs->expr_type == ExprType_INT_LIT) {
            store_imm_gen_ir(expr->rhs->int_value, lhs_reg, 0, expr_d_type,
                             cur_block);
        } else
            assert(false);

        instr = IRInstr_create_mov(
            self_reg, expr_d_type,
            IRInstrArg_create_from_expr(expr->rhs, tu->structs, rhs_reg));
    } else if (expr->expr_type == ExprType_REFERENCE) {
        instr.type = IRInstr_MOV;
    } else if (expr->expr_type == ExprType_DEREFERENCE && !load_reference) {
        instr.type = IRInstr_LOAD;
        /* an offset of 0 into the pointer */
        IRInstrArgList_push_back(
            &instr.args,
            IRInstrArg_create(IRInstrArg_IMM32, IRDataType_create(false, 32, 0),
                              IRInstrArgValue_imm_u32(0)));
    } else if (expr->expr_type == ExprType_DEREFERENCE && load_reference) {
        IRInstr_free(instr);
        return lhs_reg;
    } else {
        instr.type = IRInstr_INVALID;
        assert(false);
    }

    IRInstrList_push_back(&cur_block->instrs, instr);

    return self_reg;
}

/* size and alignment are in bytes */
static void alloca_gen_ir(const char *result_reg, struct IRDataType result_type,
                          struct IRBasicBlock *cur_block)
{
    u32 real_width = IRDataType_real_width(&result_type);

    struct IRInstr instr =
        IRInstr_create(IRInstr_ALLOCA, IRInstrArgList_init());

    IRInstrArgList_push_back(
        &instr.args, IRInstrArg_create(IRInstrArg_REG, result_type,
                                       IRInstrArgValue_reg_name(result_reg)));

    IRInstrArgList_push_back(
        &instr.args,
        IRInstrArg_create(IRInstrArg_IMM32, IRDataType_create(false, 32, 0),
                          IRInstrArgValue_imm_u32(real_width / 8)));

    IRInstrArgList_push_back(
        &instr.args,
        IRInstrArg_create(IRInstrArg_IMM32, IRDataType_create(false, 32, 0),
                          IRInstrArgValue_imm_u32(real_width / 8)));

    IRInstrList_push_back(&cur_block->instrs, instr);
}

static void ret_stmt_gen_ir(const struct RetNode *ret,
                            struct IRBasicBlock *cur_block,
                            struct IRFunc *cur_func, struct IRModule *module,
                            const struct TranslUnit *tu)
{
    struct IRInstr instr = IRInstr_create(IRInstr_RET, IRInstrArgList_init());
    const char *ret_reg = NULL;

    if (ret->value) {
        ret_reg = expr_gen_ir(ret->value, tu, cur_block, cur_func, module, NULL,
                              NULL, false);

        IRInstrArgList_push_back(
            &instr.args,
            IRInstrArg_create_from_expr(ret->value, tu->structs, ret_reg));
    } else {
        IRInstrArgList_push_back(
            &instr.args,
            IRInstrArg_create(IRInstrArg_IMM32, IRDataType_create(false, 32, 0),
                              IRInstrArgValue_imm_u32(0)));
    }

    IRInstrList_push_back(&cur_block->instrs, instr);
}

static void jmp_if_zero(const struct Expr *expr, const char *true_dest,
                        const char *false_dest, const struct TranslUnit *tu,
                        struct IRBasicBlock *cur_block, struct IRFunc *cur_func,
                        struct IRModule *module)
{
    const char *cond_reg = NULL;

    struct IRInstrArg comp_lhs;
    struct IRInstrArg comp_rhs;

    cond_reg =
        expr_gen_ir(expr, tu, cur_block, cur_func, module, NULL, NULL, false);

    comp_lhs = IRInstrArg_create_from_expr(expr, tu->structs, cond_reg);

    comp_rhs = IRInstrArg_create(
        IRInstrArg_IMM32,
        IRDataType_create_from_prim_type(expr->prim_type, expr->type_idx,
                                         expr->lvls_of_indir, tu->structs),
        IRInstrArgValue_imm_i32(0));

    IRInstrList_push_back(&cur_block->instrs,
                          IRInstr_create_cond_jmp_instr(IRInstr_JE, comp_lhs,
                                                        comp_rhs, true_dest,
                                                        false_dest));
}

static void if_stmt_gen_ir(const struct IfNode *node, struct IRFunc *cur_func,
                           struct IRModule *module, const struct TranslUnit *tu)
{
    struct IRBasicBlock *start = IRBasicBlockList_back_ptr(&cur_func->blocks);

    struct IRBasicBlock if_true = IRBasicBlock_create(
        create_string_with_num_appended("if_true_", if_else_counter),
        IRInstrList_init(), U32List_init());

    struct IRBasicBlock if_false = IRBasicBlock_create(
        create_string_with_num_appended("if_false_", if_else_counter),
        IRInstrList_init(), U32List_init());

    struct IRBasicBlock if_end = IRBasicBlock_create(
        create_string_with_num_appended("if_end_", if_else_counter),
        IRInstrList_init(), U32List_init());

    ++if_else_counter;

    jmp_if_zero(node->expr, if_false.label, if_true.label, tu, start, cur_func,
                module);

    IRBasicBlockList_push_back(&cur_func->blocks, if_true);
    if (node->body)
        block_node_gen_ir(node->body, cur_func, module, tu);
    IRInstrList_push_back(&IRBasicBlockList_back_ptr(&cur_func->blocks)->instrs,
                          IRInstr_create_str_instr(IRInstr_JMP, if_end.label));

    IRBasicBlockList_push_back(&cur_func->blocks, if_false);
    if (node->else_body)
        block_node_gen_ir(node->else_body, cur_func, module, tu);
    IRInstrList_push_back(&IRBasicBlockList_back_ptr(&cur_func->blocks)->instrs,
                          IRInstr_create_str_instr(IRInstr_JMP, if_end.label));

    IRBasicBlockList_push_back(&cur_func->blocks, if_end);
}

static void while_loop_gen_ir(const struct WhileNode *node,
                              struct IRFunc *cur_func, struct IRModule *module,
                              const struct TranslUnit *tu)
{
    struct IRBasicBlock loop_start = IRBasicBlock_create(
        create_string_with_num_appended("while_", while_loop_counter),
        IRInstrList_init(), U32List_init());

    struct IRBasicBlock body = IRBasicBlock_create(
        create_string_with_num_appended("while_body_", while_loop_counter),
        IRInstrList_init(), U32List_init());

    struct IRBasicBlock loop_end = IRBasicBlock_create(
        create_string_with_num_appended("while_end_", while_loop_counter),
        IRInstrList_init(), U32List_init());

    struct IRBasicBlock *cur_block =
        IRBasicBlockList_back_ptr(&cur_func->blocks);

    ++while_loop_counter;

    IRInstrList_push_back(
        &cur_block->instrs,
        IRInstr_create_str_instr(IRInstr_JMP, loop_start.label));

    IRBasicBlockList_push_back(&cur_func->blocks, loop_start);
    cur_block = IRBasicBlockList_back_ptr(&cur_func->blocks);

    jmp_if_zero(node->expr, loop_end.label, body.label, tu, cur_block, cur_func,
                module);

    IRBasicBlockList_push_back(&cur_func->blocks, body);
    cur_block = IRBasicBlockList_back_ptr(&cur_func->blocks);

    if (node->body)
        block_node_gen_ir(node->body, cur_func, module, tu);
    /* the cur block could have gotten updated in block_node_gen_ir */
    cur_block = IRBasicBlockList_back_ptr(&cur_func->blocks);

    IRInstrList_push_back(
        &cur_block->instrs,
        IRInstr_create_str_instr(IRInstr_JMP, loop_start.label));

    IRBasicBlockList_push_back(&cur_func->blocks, loop_end);
}

static void for_loop_gen_ir(const struct ForNode *node, struct IRFunc *cur_func,
                            struct IRModule *module,
                            const struct TranslUnit *tu)
{
    struct IRBasicBlock loop_start = IRBasicBlock_create(
        create_string_with_num_appended("for_", for_loop_counter),
        IRInstrList_init(), U32List_init());

    struct IRBasicBlock body = IRBasicBlock_create(
        create_string_with_num_appended("for_body_", for_loop_counter),
        IRInstrList_init(), U32List_init());

    struct IRBasicBlock loop_end = IRBasicBlock_create(
        create_string_with_num_appended("for_end_", for_loop_counter),
        IRInstrList_init(), U32List_init());

    struct IRBasicBlock *cur_block =
        IRBasicBlockList_back_ptr(&cur_func->blocks);

    ++for_loop_counter;

    /* the init portion will only be run once, so it isn't put inside of the
     * loop */
    expr_gen_ir(node->init, tu, cur_block, cur_func, module, NULL, NULL, false);
    IRInstrList_push_back(
        &cur_block->instrs,
        IRInstr_create_str_instr(IRInstr_JMP, loop_start.label));

    IRBasicBlockList_push_back(&cur_func->blocks, loop_start);
    cur_block = IRBasicBlockList_back_ptr(&cur_func->blocks);

    jmp_if_zero(node->condition, loop_end.label, body.label, tu, cur_block,
                cur_func, module);

    IRBasicBlockList_push_back(&cur_func->blocks, body);
    cur_block = IRBasicBlockList_back_ptr(&cur_func->blocks);

    if (node->body)
        block_node_gen_ir(node->body, cur_func, module, tu);
    /* the cur block could have gotten updated in block_node_gen_ir */
    cur_block = IRBasicBlockList_back_ptr(&cur_func->blocks);

    /* the inc part runs at the end of each iteration */
    expr_gen_ir(node->inc, tu, cur_block, cur_func, module, NULL, NULL, false);

    IRInstrList_push_back(
        &cur_block->instrs,
        IRInstr_create_str_instr(IRInstr_JMP, loop_start.label));

    IRBasicBlockList_push_back(&cur_func->blocks, loop_end);
}

/* returns the mangled name, and updates cur_func->vregs and the name_mangles
 * list */
static const char *declarator_name_mangle(const char *name,
                                          struct IRFunc *cur_func)
{
    struct DynamicStr new_name = DynamicStr_init();

    u32 idx = IRNameMangleList_find(&name_mangles, name);
    const struct IRNameMangle *name_mangle = NULL;

    if (IRNameMangleList_find(&name_mangles, name) == m_u32_max) {
        IRNameMangleList_push_back(
            &name_mangles,
            IRNameMangle_create(make_str_copy(name), ConstStringList_init()));
        idx = name_mangles.size - 1;
    }
    name_mangle = &name_mangles.elems[idx];

    DynamicStr_append_printf(&new_name, "%s.nr%u", name,
                             name_mangle->new_names.size);

    StringList_push_back(&cur_func->vregs, new_name.str);

    ConstStringList_push_back(
        &IRNameMangleList_back_ptr(&name_mangles)->new_names, new_name.str);

    return new_name.str;
}

/* returns the mangled name of the declarator */
static const char *
declarator_gen_ir(enum PrimitiveType type, u32 type_idx, u32 indir,
                  const char *name, const struct Expr *init,
                  struct IRBasicBlock *cur_block, struct IRFunc *cur_func,
                  struct IRModule *module, const struct TranslUnit *tu)
{
    struct IRDataType var_type;
    /* the reg holding the result of alloca */
    struct DynamicStr init_reg = DynamicStr_init();
    const char *var_reg = NULL;

    var_type =
        IRDataType_create_from_prim_type(type, type_idx, indir, tu->structs);

    var_reg = declarator_name_mangle(name, cur_func);
    DynamicStr_append_printf(&init_reg, "%s.init", var_reg);

    alloca_gen_ir(var_reg,
                  IRDataType_create(var_type.is_signed, var_type.width,
                                    var_type.lvls_of_indir + 1),
                  cur_block);

    if (!init) {
        DynamicStr_free(init_reg);
    } else {
        StringList_push_back(&cur_func->vregs, init_reg.str);
        expr_gen_ir(init, tu, cur_block, cur_func, module, init_reg.str,
                    &var_type, false);

        store_gen_ir(init_reg.str, var_reg, 0, var_type, cur_block);
    }

    return var_reg;
}

static void var_decl_gen_ir(const struct VarDeclNode *node,
                            struct IRBasicBlock *cur_block,
                            struct IRFunc *cur_func, struct IRModule *module,
                            const struct TranslUnit *tu)
{
    u32 i;

    for (i = 0; i < node->decls.size; i++) {
        const struct Declarator *decl = &node->decls.elems[i];

        declarator_gen_ir(node->type, node->type_idx, decl->lvls_of_indir,
                          decl->ident, decl->value, cur_block, cur_func, module,
                          tu);
    }
}

static void ast_node_gen_ir(const struct ASTNode *node,
                            struct IRBasicBlock *cur_block,
                            struct IRFunc *cur_func, struct IRModule *module,
                            const struct TranslUnit *tu)
{
    if (node->type == ASTType_EXPR) {
        struct Expr *expr = NULL;

        expr = ((struct ExprNode *)node->node_struct)->expr;

        expr_gen_ir(expr, tu, cur_block, cur_func, module, NULL, NULL, false);
    } else if (node->type == ASTType_RETURN) {
        ret_stmt_gen_ir(node->node_struct, cur_block, cur_func, module, tu);
    } else if (node->type == ASTType_IF_STMT) {
        if_stmt_gen_ir(node->node_struct, cur_func, module, tu);
    } else if (node->type == ASTType_WHILE_STMT) {
        while_loop_gen_ir(node->node_struct, cur_func, module, tu);
    } else if (node->type == ASTType_FOR_STMT) {
        for_loop_gen_ir(node->node_struct, cur_func, module, tu);
    } else if (node->type == ASTType_VAR_DECL) {
        var_decl_gen_ir(node->node_struct, cur_block, cur_func, module, tu);
    } else if (node->type == ASTType_BLOCK) {
        block_node_gen_ir(node->node_struct, cur_func, module, tu);
    } else {
        printf("unsupported node at %u,%u\n", node->line_num, node->column_num);
        assert(false);
    }
}

static void block_node_gen_ir(struct BlockNode *block, struct IRFunc *cur_func,
                              struct IRModule *module,
                              const struct TranslUnit *tu)
{
    u32 i;

    struct IRBasicBlock *cur_block =
        IRBasicBlockList_back_ptr(&cur_func->blocks);

    for (i = 0; i < block->nodes.size; i++) {
        ast_node_gen_ir(&block->nodes.elems[i], cur_block, cur_func, module,
                        tu);
        cur_block = IRBasicBlockList_back_ptr(&cur_func->blocks);
    }
}

static struct IRFuncArgList func_node_get_args(const struct FuncDeclNode *func,
                                               const struct TranslUnit *tu)
{
    u32 i;

    struct IRFuncArgList args = IRFuncArgList_init();

    for (i = 0; i < func->args.size; i++) {
        struct IRFuncArg arg;
        struct Declarator *node_arg = &func->args.elems[i]->decls.elems[0];

        struct DynamicStr arg_name = DynamicStr_init();
        DynamicStr_append_printf(&arg_name, "%s", node_arg->ident);

        arg = IRFuncArg_create(
            IRDataType_create_from_prim_type(
                func->args.elems[i]->type, func->args.elems[i]->type_idx,
                func->args.elems[i]->decls.elems[0].lvls_of_indir, tu->structs),
            arg_name.str);

        IRFuncArgList_push_back(&args, arg);
    }

    return args;
}

/* creates a copy of each func arg on the stack to make it act like a regular
 * variable */
static void func_setup_args(struct IRFunc *func, struct IRModule *module,
                            struct IRBasicBlock *start,
                            const struct TranslUnit *tu)
{
    u32 i;

    for (i = 0; i < func->args.size; i++) {
        const struct IRFuncArg *arg = &func->args.elems[i];
        char *arg_cpy = make_str_copy(arg->name);
        struct IRInstr store_instr =
            IRInstr_create(IRInstr_STORE, IRInstrArgList_init());

        const char *vreg = declarator_gen_ir(
            IRDataType_to_prim_type(&arg->type), 0, arg->type.lvls_of_indir,
            arg->name, NULL, start, func, module, tu);

        StringList_push_back(&func->vregs, arg_cpy);

        IRInstrArgList_push_back(
            &store_instr.args,
            IRInstrArg_create(IRInstrArg_REG, arg->type,
                              IRInstrArgValue_reg_name(arg_cpy)));
        IRInstrArgList_push_back(
            &store_instr.args,
            IRInstrArg_create(IRInstrArg_REG,
                              IRDataType_create(arg->type.is_signed,
                                                arg->type.width,
                                                arg->type.lvls_of_indir + 1),
                              IRInstrArgValue_reg_name(vreg)));
        IRInstrArgList_push_back(
            &store_instr.args,
            IRInstrArg_create(IRInstrArg_IMM32, IRDataType_create(false, 32, 0),
                              IRInstrArgValue_imm_u32(0)));
        IRInstrList_push_back(&start->instrs, store_instr);
    }
}

static struct IRFunc func_node_gen_ir(const struct FuncDeclNode *func,
                                      struct IRModule *module,
                                      const struct TranslUnit *tu)
{
    struct IRFunc ir_func = IRFunc_create(
        make_str_copy(func->name), IRFuncMods_create_from_func_node(func, tu),
        IRDataType_create_from_prim_type(func->ret_type, func->ret_type_idx,
                                         func->ret_lvls_of_indir, tu->structs),
        func_node_get_args(func, tu), IRBasicBlockList_init(),
        StringList_init());

    if (!func->body)
        return ir_func;

    IRBasicBlockList_push_back(&ir_func.blocks,
                               IRBasicBlock_create(make_str_copy("start"),
                                                   IRInstrList_init(),
                                                   U32List_init()));

    reg_counter = 0;
    if_else_counter = 0;
    while_loop_counter = 0;
    for_loop_counter = 0;

    name_mangles = IRNameMangleList_init();

    func_setup_args(&ir_func, module,
                    IRBasicBlockList_back_ptr(&ir_func.blocks), tu);

    block_node_gen_ir(func->body, &ir_func, module, tu);
    IRFunc_move_allocas_to_top(&ir_func);
    IRFunc_get_blocks_imm_doms(&ir_func);

    while (name_mangles.size > 0) {
        IRNameMangleList_pop_back(&name_mangles, IRNameMangle_free);
    }
    IRNameMangleList_free(&name_mangles);

    return ir_func;
}

static void get_array_lits(struct IRModule *module, const struct TranslUnit *tu)
{
    u32 i;
    struct ArrayLitList array_lits = ArrayLitList_init();

    BlockNode_get_array_lits(tu->ast, &array_lits);

    for (i = 0; i < array_lits.size; i++) {
        struct DynamicStr name = DynamicStr_init();
        struct IRArrayLit lit = IRArrayLit_init();
        const struct ArrayLit *other_lit = &array_lits.elems[i];

        /* a dollar sign is appended to prevent naming conflicts with C
         * identifiers */
        DynamicStr_append_printf(&name, "array_lit_%u$", i);

        lit.name = name.str;
        lit.elem_width = other_lit->elem_size * 8;
        IRArrayLit_append_elems(&lit, other_lit);
        IRArrayLitList_push_back(&module->array_lits, lit);
    }

    ArrayLitList_free(&array_lits);
}

static struct IRModule ast_gen_ir(const struct TranslUnit *tu)
{
    u32 i;

    struct IRModule module = IRModule_init();

    get_array_lits(&module, tu);

    for (i = 0; i < tu->ast->nodes.size; i++) {
        assert(tu->ast->nodes.elems[i].type == ASTType_FUNC);

        IRFuncList_push_back(
            &module.funcs,
            func_node_gen_ir(tu->ast->nodes.elems[i].node_struct, &module, tu));
    }

    return module;
}

struct IRModule IRGen_generate(const struct TranslUnit *tu)
{
    return ast_gen_ir(tu);
}
