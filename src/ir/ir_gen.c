#include "ir_gen.h"
#include "basic_block.h"
#include "data_types.h"
#include "func.h"
#include "instr.h"
#include "module.h"
#include "../utils/dyn_str.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

u32 reg_counter = 0;

static void block_node_gen_ir(const struct BlockNode *block,
        struct IRBasicBlockList *basic_blocks, const struct TranslUnit *tu);

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
        const struct TranslUnit *tu, struct IRBasicBlock *cur_block) {

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
        lhs_reg = expr_gen_ir(expr->lhs, tu, cur_block);
    }

    if (expr->rhs) {
        rhs_reg = expr_gen_ir(expr->rhs, tu, cur_block);
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
    else {
        instr.type = IRInstr_INVALID;
        assert(false);
    }

    IRInstrList_push_back(&cur_block->instrs, instr);

    m_free(lhs_reg);
    m_free(rhs_reg);

    return self_reg.str;

}

static void ret_stmt_gen_ir(const struct RetNode *ret,
        struct IRBasicBlock *cur_block, const struct TranslUnit *tu) {

    struct IRInstr instr = IRInstr_create(IRInstr_RET, IRInstrArgList_init());
    char *ret_reg = NULL;

    if (ret->value) {
        ret_reg = expr_gen_ir(ret->value, tu, cur_block);

        IRInstrArgList_push_back(&instr.args,
                IRInstrArg_create_from_expr(
                    ret->value, tu->structs, make_str_copy(ret_reg)
                    ));
    }
    else {
        IRInstrArgList_push_back(&instr.args,
                IRInstrArg_create(
                    IRInstrArg_IMM32, IRDataType_create(false, 32, 0),
                    IRInstrArgValue_imm_u32(0)));
    }

    IRInstrList_push_back(&cur_block->instrs, instr);

    m_free(ret_reg);

}

static void if_stmt_gen_ir(const struct IfNode *node,
        struct IRBasicBlockList *basic_blocks, const struct TranslUnit *tu) {

    struct IRBasicBlock *cur_block = IRBasicBlockList_back_ptr(basic_blocks);

    struct IRBasicBlock if_true = IRBasicBlock_create(
            cur_block->parent, make_str_copy("if_true"), IRInstrList_init()
            );

    struct IRBasicBlock if_false = IRBasicBlock_create(
            cur_block->parent, make_str_copy("if_false"), IRInstrList_init()
            );

    struct IRBasicBlock if_end = IRBasicBlock_create(
            cur_block->parent, make_str_copy("if_end"), IRInstrList_init()
            );

    char *cond_reg = NULL;

    cond_reg = expr_gen_ir(node->expr, tu, cur_block);

    /* jmp to the false node if the condition resulted in a 0, and to the
     * true node otherwise */
    {
        struct IRInstrArg comp_lhs = IRInstrArg_create_from_expr(
                    node->expr, tu->structs, make_str_copy(cond_reg)
                );

        struct IRInstrArg comp_rhs = IRInstrArg_create(
                    IRInstrArg_IMM32, IRDataType_create_from_prim_type(
                        node->expr->prim_type, node->expr->type_idx,
                        node->expr->lvls_of_indir, tu->structs
                        ),

                    IRInstrArgValue_imm_i32(0)
                );

        IRInstrList_push_back(&cur_block->instrs,
                IRInstr_create_cond_jmp_instr(IRInstr_JE,
                    comp_lhs,
                    comp_rhs,
                    make_str_copy(if_false.label))
                );
    }
    IRInstrList_push_back(&cur_block->instrs,
            IRInstr_create_str_instr(IRInstr_JMP, make_str_copy(if_true.label))
            );

    IRBasicBlockList_push_back(basic_blocks, if_true);
    if (node->body)
        block_node_gen_ir(node->body, basic_blocks, tu);
    IRInstrList_push_back(&IRBasicBlockList_back_ptr(basic_blocks)->instrs,
            IRInstr_create_str_instr(IRInstr_JMP, make_str_copy(if_end.label))
            );

    IRBasicBlockList_push_back(basic_blocks, if_false);
    if (node->else_body)
        block_node_gen_ir(node->else_body, basic_blocks, tu);
    IRInstrList_push_back(&IRBasicBlockList_back_ptr(basic_blocks)->instrs,
            IRInstr_create_str_instr(IRInstr_JMP, make_str_copy(if_end.label))
            );

    IRBasicBlockList_push_back(basic_blocks, if_end);

    m_free(cond_reg);

}

static void ast_node_gen_ir(const struct ASTNode *node,
        struct IRBasicBlock *cur_block, struct IRBasicBlockList *basic_blocks,
        const struct TranslUnit *tu) {

    if (node->type == ASTType_EXPR) {
        struct Expr *expr = NULL;

        expr = ((struct ExprNode*)node->node_struct)->expr;

        free(expr_gen_ir(expr, tu, cur_block));
    }
    else if (node->type == ASTType_RETURN) {
        ret_stmt_gen_ir(node->node_struct, cur_block, tu);
    }
    else if (node->type == ASTType_IF_STMT) {
        if_stmt_gen_ir(node->node_struct, basic_blocks, tu);
    }
    else if (node->type == ASTType_BLOCK) {
        block_node_gen_ir(node->node_struct, basic_blocks, tu);
    }
    else {
        printf("unsupported node at %u,%u\n",
                node->line_num, node->column_num);
        assert(false);
    }

}

static void block_node_gen_ir(const struct BlockNode *block,
        struct IRBasicBlockList *basic_blocks, const struct TranslUnit *tu) {

    u32 i;

    struct IRBasicBlock *cur_block = IRBasicBlockList_back_ptr(basic_blocks);

    for (i = 0; i < block->nodes.size; i++) {

        ast_node_gen_ir(&block->nodes.elems[i], cur_block, basic_blocks, tu);
        cur_block = IRBasicBlockList_back_ptr(basic_blocks);

    }

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

    struct IRFunc ir_func = IRFunc_create(
            make_str_copy(func->name),
            IRDataType_create_from_prim_type(
                func->ret_type, func->ret_type_idx, func->ret_lvls_of_indir,
                tu->structs
                ),
            func_node_get_args(func, tu),
            IRBasicBlockList_init()
            );

    IRBasicBlockList_push_back(&ir_func.blocks,
            IRBasicBlock_create(
                &ir_func, make_str_copy("start"), IRInstrList_init()
                )
            );

    block_node_gen_ir(func->body, &ir_func.blocks, tu);

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
