#include "ast.h"
#include "bool.h"
#include "safe_mem.h"
#include "type_sizes.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

unsigned PrimitiveType_size(enum PrimitiveType type) {

    switch (type) {

    case PrimType_INT:
        return m_TypeSize_int;

    case PrimType_INVALID:
        assert(false);

    }

}

enum PrimitiveType PrimitiveType_promote(enum PrimitiveType type) {

    switch (type) {

    case PrimType_INT:
        return PrimType_INT;

    case PrimType_INVALID:
        assert(false);

    }

}

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
    expr.line_num = 0;
    expr.column_num = 0;
    expr.src_start = NULL;
    expr.src_len = 0;
    expr.lhs = NULL;
    expr.rhs = NULL;
    expr.expr_type = ExprType_INVALID;
    expr.int_value = 0;
    return expr;

}

struct Expr Expr_create(unsigned line_num, unsigned column_num,
        const char *src_start, unsigned src_len, struct Expr *lhs,
        struct Expr *rhs, enum PrimitiveType lhs_type,
        enum PrimitiveType rhs_type, u32 int_value, enum ExprType expr_type) {

    struct Expr expr;
    expr.line_num = line_num;
    expr.column_num = column_num;
    expr.src_start = src_start;
    expr.src_len = src_len;
    expr.lhs = lhs;
    expr.rhs = rhs;
    expr.lhs_type = lhs_type;
    expr.rhs_type = rhs_type;
    expr.int_value = int_value;
    expr.expr_type = expr_type;
    return expr;

}

struct Expr Expr_create_w_tok(struct Token token, struct Expr *lhs,
        struct Expr *rhs, enum PrimitiveType lhs_type,
        enum PrimitiveType rhs_type, u32 int_value, enum ExprType expr_type) {

    return Expr_create(token.line_num, token.column_num, token.src_start,
            token.src_len, lhs, rhs, lhs_type, rhs_type, int_value, expr_type);

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

enum PrimitiveType Expr_type(struct Expr *self) {

    if (self->rhs) {
        enum PrimitiveType lhs_prom = PrimitiveType_promote(self->lhs_type);
        /*
        enum PrimitiveType rhs_prom = PrimitiveType_promote(self->rhs_type);
        */

        /* Time for C arithmetic type conversions! Yayyyy! */
        /* Actually not yet cuz i haven't added any types other than int yet */
        return lhs_prom;
    }
    else {
        return PrimitiveType_promote(self->lhs_type);
    }

}

u32 Expr_evaluate(struct Expr *self) {

    u32 lhs_val = self->lhs ? Expr_evaluate(self->lhs) :
        !self->rhs ? self->int_value : 0;
    u32 rhs_val = self->rhs ? Expr_evaluate(self->rhs) : 0;

    assert(!(!self->lhs && self->rhs));

    switch (self->expr_type) {

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
