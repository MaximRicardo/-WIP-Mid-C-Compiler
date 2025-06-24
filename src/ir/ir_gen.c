#include "ir_gen.h"
#include "basic_block.h"
#include "data_types.h"
#include "func.h"
#include "instr.h"
#include "module.h"
#include "../utils/dyn_str.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

u32 reg_counter = 0;

static char* make_str_copy(const char *str) {

    u32 len;
    char *new_str = NULL;

    if (!str)
        return NULL;

    len = strlen(str);
    new_str = safe_malloc((len+1)*sizeof(*new_str));

    strcpy(new_str, str);

    return new_str;

}

/* returns the register the result of the expr got put into */
static char* expr_gen_ir(const struct Expr *expr,
        const struct TranslUnit *tu, struct IRBasicBlock *block) {

    struct IRInstr instr = IRInstr_init();

    char *lhs_reg = NULL;
    char *rhs_reg = NULL;
    struct DynamicStr self_reg = DynamicStr_init();

    if (expr->expr_type == ExprType_INT_LIT) {
        DynamicStr_free(self_reg);
        IRInstr_free(instr);
        return NULL;
    }

    if (expr->lhs) {
        lhs_reg = expr_gen_ir(expr->lhs, tu, block);
    }

    if (expr->rhs) {
        rhs_reg = expr_gen_ir(expr->rhs, tu, block);
    }

    DynamicStr_append_printf(&self_reg, "%u", reg_counter++);

    /* the operands go in this order:
     * self, lhs, rhs
     */

    IRInstrArgList_push_back(&instr.args,
            IRInstrArg_create_from_expr(
                expr, tu->structs, make_str_copy(self_reg.str)
                ));

    IRInstrArgList_push_back(&instr.args,
            IRInstrArg_create_from_expr(
                expr->lhs, tu->structs, make_str_copy(lhs_reg)
                ));

    IRInstrArgList_push_back(&instr.args,
            IRInstrArg_create_from_expr(
                expr->rhs, tu->structs, make_str_copy(rhs_reg)
                ));

    if (expr->expr_type == ExprType_PLUS) {
        instr.type = IRInstr_ADD;
    }
    else if (expr->expr_type == ExprType_MINUS) {
        instr.type = IRInstr_SUB;
    }
    else if (expr->expr_type == ExprType_MUL) {
        instr.type = IRInstr_MUL;
    }
    else if (expr->expr_type == ExprType_DIV) {
        instr.type = IRInstr_DIV;
    }
    else
        instr.type = IRInstr_INVALID;

    IRInstrList_push_back(&block->instrs, instr);

    m_free(lhs_reg);
    m_free(rhs_reg);

    return self_reg.str;

}

static struct IRFuncArgList func_node_get_args(const struct FuncDeclNode *func,
        const struct TranslUnit *tu) {

    u32 i;

    struct IRFuncArgList args = IRFuncArgList_init();

    for (i = 0; i < func->args.size; i++) {

        struct IRFuncArg arg;

        struct DynamicStr arg_name = DynamicStr_init();
        DynamicStr_append_printf(&arg_name, "arg.%u$", i);

        arg = IRFuncArg_create(
                IRDataType_create_from_prim_type(
                    func->args.elems[i]->type, func->args.elems[i]->type_idx,
                    func->args.elems[i]->decls.elems[0].lvls_of_indir,
                    tu->structs
                    ),
                arg_name.str
                );

        IRFuncArgList_push_back(&args, arg);

    }

    return args;

}

static struct IRFunc func_node_gen_ir(const struct FuncDeclNode *func,
        const struct TranslUnit *tu) {

    u32 i;

    struct IRBasicBlock *cur_block = NULL;

    const struct ASTNodeList *nodes = &func->body->nodes;

    struct IRFunc ir_func = IRFunc_create(
            make_str_copy(func->name),
            IRDataType_create_from_prim_type(
                func->ret_type, func->ret_type_idx, func->ret_lvls_of_indir,
                tu->structs
                ),
            func_node_get_args(func, tu),
            IRBasicBlockList_init()
            );

    IRBasicBlockList_push_back(&ir_func.blocks, IRBasicBlock_init());
    cur_block = IRBasicBlockList_back_ptr(&ir_func.blocks);

    for (i = 0; i < nodes->size; i++) {

        struct Expr *expr = NULL;

        if (nodes->elems[i].type != ASTType_EXPR)
            continue;

        expr = ((struct ExprNode*)nodes->elems[i].node_struct)->expr;

        free(expr_gen_ir(expr, tu, cur_block));

    }

    return ir_func;

}

static struct IRModule ast_gen_ir(const struct TranslUnit *tu) {

    u32 i;

    struct IRModule module = IRModule_init();

    for (i = 0; i < tu->ast->nodes.size; i++) {

        assert(tu->ast->nodes.elems[i].type == ASTType_FUNC);

        if (!((struct FuncDeclNode*)tu->ast->nodes.elems[i].node_struct)->body)
            continue;

        IRFuncList_push_back(&module.funcs,
                func_node_gen_ir(tu->ast->nodes.elems[i].node_struct, tu));

    }

    return module;

}

struct IRModule IRGen_generate(const struct TranslUnit *tu) {

    return ast_gen_ir(tu);

}
