#include "ast.h"
#include "bool.h"
#include "safe_mem.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

struct ASTNode ASTNode_init(void) {

    struct ASTNode node;
    node.type = AstType_INVALID;
    return node;

}

struct ASTNode ASTNode_create(enum ASTNodeType type) {

    struct ASTNode node;
    node.type = type;
    return node;

}

struct Expr Expr_init(void) {

    struct Expr expr;
    expr.lhs = NULL;
    expr.rhs = NULL;
    expr.type = ExprType_INVALID;
    expr.int_value = 0;
    return expr;

}

struct Expr Expr_create(struct Expr *lhs, struct Expr *rhs,
        enum PrimitiveType lhs_type, enum PrimitiveType rhs_type,
        u32 int_value, enum ExprType type) {

    struct Expr expr;
    expr.lhs = lhs;
    expr.rhs = rhs;
    expr.lhs_type = lhs_type;
    expr.rhs_type = rhs_type;
    expr.int_value = int_value;
    expr.type = type;
    return expr;

}

void Expr_free(struct Expr *self) {

    if (!self)
        return;

    if (self->lhs)
        Expr_free(self->lhs);
    if (self->rhs)
        Expr_free(self->rhs);

    m_free(self);

}

u32 Expr_evaluate(struct Expr *self) {

    u32 lhs_val = self->lhs ? Expr_evaluate(self->lhs) :
        !self->rhs ? self->int_value : 0;
    u32 rhs_val = self->rhs ? Expr_evaluate(self->rhs) : 0;

    assert(!(!self->lhs && self->rhs));

    switch (self->type) {

    case ExprType_PLUS:
        return lhs_val+rhs_val;

    case ExprType_MINUS:
        return lhs_val-rhs_val;

    case ExprType_MUL:
        return lhs_val*rhs_val;

    case ExprType_DIV:
        return lhs_val/rhs_val;

    case ExprType_INT_LIT:
        return lhs_val;

    default:
        assert(false);

    }

}

struct ExprStmt ExprStmt_init(void) {

    struct ExprStmt expr_stmt;
    expr_stmt.expr = NULL;
    return expr_stmt;

}

struct ExprStmt ExprStmt_create(struct Expr *expr) {

    struct ExprStmt expr_stmt;
    expr_stmt.expr = expr;
    return expr_stmt;

}
