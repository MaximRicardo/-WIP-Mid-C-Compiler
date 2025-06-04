#pragma once

#include "ints.h"

enum PrimitiveType {

    PrimType_INVALID,

    PrimType_INT

};

enum ASTNodeType {

    AstType_INVALID,

    AstType_EXPR_STMT

};

struct ASTNode {

    enum ASTNodeType type;

};

struct ASTNode ASTNode_init(void);
struct ASTNode ASTNode_create(enum ASTNodeType type);

enum ExprType {

    ExprType_INVALID,

    /* Literals */
    ExprType_INT_LIT,

    /* Binary operators */
    ExprType_PLUS,
    ExprType_MINUS,
    ExprType_MUL,
    ExprType_DIV

};

struct Expr {

    struct Expr *lhs, *rhs;
    enum PrimitiveType lhs_type, rhs_type;
    u32 int_value;
    enum ExprType type;

};

struct Expr Expr_init(void);
struct Expr Expr_create(struct Expr *lhs, struct Expr *rhs,
        enum PrimitiveType lhs_type, enum PrimitiveType rhs_type,
        u32 int_value, enum ExprType type);
void Expr_free(struct Expr *self);
u32 Expr_evaluate(struct Expr *expr);

struct ExprStmt {

    struct Expr *expr;

};

struct ExprStmt ExprStmt_init(void);
struct ExprStmt ExprStmt_create(struct Expr *expr);
