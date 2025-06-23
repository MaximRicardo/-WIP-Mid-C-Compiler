#include "const_fold.h"
#include "array_lit.h"
#include "ast.h"
#include "prim_type.h"
#include "safe_mem.h"
#include <stdio.h>

void Expr_const_fold(struct Expr **expr, const struct StructList *structs) {

    u32 i;

    if (!*expr)
        return;

    if (Expr_statically_evaluatable(*expr)) {
        enum PrimitiveType old_type = (*expr)->prim_type;
        u32 value = Expr_evaluate(*expr, structs);
        struct Expr old_expr = **expr;

        Expr_recur_free_w_self(*expr);
        *expr = safe_malloc(sizeof(**expr));
        **expr = Expr_create(old_expr.line_num, old_expr.column_num,
                old_expr.src_start, old_expr.src_len, old_expr.file_path,
                NULL, NULL, old_type, old_type, 0, 0,
                ExprPtrList_init(), value, ArrayLit_init(), 0,
                ExprType_INT_LIT, false, 0);
    }
    else {
        if ((*expr)->lhs)
            Expr_const_fold(&(*expr)->lhs, structs);
        if ((*expr)->rhs)
            Expr_const_fold(&(*expr)->rhs, structs);
    }

    for (i = 0; i < (*expr)->args.size; i++) {
        Expr_const_fold(&(*expr)->args.elems[i], structs);
    }

}

void ExprNode_const_fold(struct ExprNode *self,
        const struct StructList *structs) {

    Expr_const_fold(&self->expr, structs);

}

void Declarator_const_fold(struct Declarator *self,
        const struct StructList *structs) {

    Expr_const_fold(&self->value, structs);

}

void VarDeclNode_const_fold(struct VarDeclNode *self,
        const struct StructList *structs) {

    u32 i;

    for (i = 0; i < self->decls.size; i++) {
        Declarator_const_fold(&self->decls.elems[i], structs);
    }

}

void FuncDeclNode_const_fold(struct FuncDeclNode *self,
        const struct StructList *structs) {

    if (self->body)
        BlockNode_const_fold(self->body, structs);

}

void RetNode_const_fold(struct RetNode *self,
        const struct StructList *structs) {

    Expr_const_fold(&self->value, structs);

}

void IfNode_const_fold(struct IfNode *self, const struct StructList *structs) {

    Expr_const_fold(&self->expr, structs);
    if (self->body)
        BlockNode_const_fold(self->body, structs);
    if (self->else_body)
        BlockNode_const_fold(self->else_body, structs);

}

void WhileNode_const_fold(struct WhileNode *self,
        const struct StructList *structs) {

    Expr_const_fold(&self->expr, structs);
    if (self->body)
        BlockNode_const_fold(self->body, structs);

}

void ForNode_const_fold(struct ForNode *self,
        const struct StructList *structs) {

    Expr_const_fold(&self->init, structs);
    Expr_const_fold(&self->condition, structs);
    Expr_const_fold(&self->inc, structs);
    if (self->body)
        BlockNode_const_fold(self->body, structs);

}

void ASTNode_const_fold(struct ASTNode *self,
        const struct StructList *structs) {

    if (self->node_struct) {
        if (self->type == ASTType_EXPR)
            ExprNode_const_fold(self->node_struct, structs);
        else if (self->type == ASTType_VAR_DECL)
            VarDeclNode_const_fold(self->node_struct, structs);
        else if (self->type == ASTType_FUNC)
            FuncDeclNode_const_fold(self->node_struct, structs);
        else if (self->type == ASTType_BLOCK)
            BlockNode_const_fold(self->node_struct, structs);
        else if (self->type == ASTType_RETURN)
            RetNode_const_fold(self->node_struct, structs);
        else if (self->type == ASTType_IF_STMT)
            IfNode_const_fold(self->node_struct, structs);
        else if (self->type == ASTType_WHILE_STMT)
            WhileNode_const_fold(self->node_struct, structs);
        else if (self->type == ASTType_FOR_STMT)
            ForNode_const_fold(self->node_struct, structs);
    }

}

void BlockNode_const_fold(struct BlockNode *self,
        const struct StructList *structs) {

    u32 i;

    for (i = 0; i < self->nodes.size; i++) {
        ASTNode_const_fold(&self->nodes.elems[i], structs);
    }

}
