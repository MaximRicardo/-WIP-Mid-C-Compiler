#pragma once

/* constant folding pre-computes any expressions that are statically
 * evalutable. */

#include "ast.h"

void Expr_const_fold(struct Expr **expr);
void ExprNode_const_fold(struct ExprNode *self);
void Declarator_const_fold(struct Declarator *self);
void VarDeclNode_const_fold(struct VarDeclNode *self);
void FuncDeclNode_const_fold(struct FuncDeclNode *self);
void RetNode_const_fold(struct RetNode *self);
void IfNode_const_fold(struct IfNode *self);
void WhileNode_const_fold(struct WhileNode *self);
void ForNode_const_fold(struct ForNode *self);
void ASTNode_const_fold(struct ASTNode *self);
void BlockNode_const_fold(struct BlockNode *self);
