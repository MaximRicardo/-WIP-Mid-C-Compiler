#pragma once

/* constant folding pre-computes any expressions that are statically
 * evalutable. */

#include "ast.h"

void Expr_const_fold(struct Expr **expr, const struct StructList *structs);
void ExprNode_const_fold(struct ExprNode *self,
        const struct StructList *structs);
void Declarator_const_fold(struct Declarator *self,
        const struct StructList *structs);
void VarDeclNode_const_fold(struct VarDeclNode *self,
        const struct StructList *structs);
void FuncDeclNode_const_fold(struct FuncDeclNode *self,
        const struct StructList *structs);
void RetNode_const_fold(struct RetNode *self,
        const struct StructList *structs);
void IfNode_const_fold(struct IfNode *self, const struct StructList *structs);
void WhileNode_const_fold(struct WhileNode *self,
        const struct StructList *structs);
void ForNode_const_fold(struct ForNode *self,
        const struct StructList *structs);
void ASTNode_const_fold(struct ASTNode *self,
        const struct StructList *structs);
void BlockNode_const_fold(struct BlockNode *self,
        const struct StructList *structs);
