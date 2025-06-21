#include "const_fold.h"
#include "array_lit.h"
#include "ast.h"
#include "prim_type.h"
#include "safe_mem.h"

void Expr_const_fold(struct Expr **expr) {

    if (!*expr)
        return;

    if (Expr_statically_evaluatable(*expr)) {
        u32 value = Expr_evaluate(*expr);
        struct Expr old_expr = **expr;

        Expr_recur_free_w_self(*expr);
        *expr = safe_malloc(sizeof(**expr));
        **expr = Expr_create(old_expr.line_num, old_expr.column_num,
                old_expr.src_start, old_expr.src_len, old_expr.file_path,
                NULL, NULL, 0, 0, PrimType_INVALID, PrimType_INVALID,
                ExprPtrList_init(), value, ArrayLit_init(), 0,
                ExprType_INT_LIT, false, 0);
    }
    else {
        if ((*expr)->lhs)
            Expr_const_fold(&(*expr)->lhs);
        if ((*expr)->rhs)
            Expr_const_fold(&(*expr)->rhs);
    }

}

void ExprNode_const_fold(struct ExprNode *self) {

    Expr_const_fold(&self->expr);

}

void Declarator_const_fold(struct Declarator *self) {

    Expr_const_fold(&self->value);

}

void VarDeclNode_const_fold(struct VarDeclNode *self) {

    u32 i;

    for (i = 0; i < self->decls.size; i++) {
        Declarator_const_fold(&self->decls.elems[i]);
    }

}

void FuncDeclNode_const_fold(struct FuncDeclNode *self) {

    if (self->body)
        BlockNode_const_fold(self->body);

}

void RetNode_const_fold(struct RetNode *self) {

    Expr_const_fold(&self->value);

}

void IfNode_const_fold(struct IfNode *self) {

    Expr_const_fold(&self->expr);
    if (self->body)
        BlockNode_const_fold(self->body);
    if (self->else_body)
        BlockNode_const_fold(self->else_body);

}

void WhileNode_const_fold(struct WhileNode *self) {

    Expr_const_fold(&self->expr);
    if (self->body)
        BlockNode_const_fold(self->body);

}

void ForNode_const_fold(struct ForNode *self) {

    Expr_const_fold(&self->init);
    Expr_const_fold(&self->condition);
    Expr_const_fold(&self->inc);
    if (self->body)
        BlockNode_const_fold(self->body);

}

void ASTNode_const_fold(struct ASTNode *self) {

    if (self->node_struct) {
        if (self->type == ASTType_EXPR)
            ExprNode_const_fold(self->node_struct);
        else if (self->type == ASTType_VAR_DECL)
            VarDeclNode_const_fold(self->node_struct);
        else if (self->type == ASTType_FUNC)
            FuncDeclNode_const_fold(self->node_struct);
        else if (self->type == ASTType_BLOCK)
            BlockNode_const_fold(self->node_struct);
        else if (self->type == ASTType_RETURN)
            RetNode_const_fold(self->node_struct);
        else if (self->type == ASTType_IF_STMT)
            IfNode_const_fold(self->node_struct);
        else if (self->type == ASTType_WHILE_STMT)
            WhileNode_const_fold(self->node_struct);
        else if (self->type == ASTType_FOR_STMT)
            ForNode_const_fold(self->node_struct);
    }

}

void BlockNode_const_fold(struct BlockNode *self) {

    u32 i;

    for (i = 0; i < self->nodes.size; i++) {
        ASTNode_const_fold(&self->nodes.elems[i]);
    }

}
