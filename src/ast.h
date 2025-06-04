#pragma once

#include "ints.h"
#include "token.h"

enum PrimitiveType {

    PrimType_INVALID,

    PrimType_INT

};

unsigned PrimitiveType_size(enum PrimitiveType type);
enum PrimitiveType PrimitiveType_promote(enum PrimitiveType type);

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
    ExprType_DIV,

    /* Only used in the shunting yard algorithm */
    ExprType_PAREN

};

struct Expr {

    unsigned line_num, column_num;
    /* Region of the source code that corresponds to this node's corresponding
     * token */
    const char *src_start; unsigned src_len;

    struct Expr *lhs, *rhs;
    enum PrimitiveType lhs_type, rhs_type;

    u32 int_value;

    enum ExprType expr_type;

};

struct Expr Expr_init(void);
struct Expr Expr_create(unsigned line_num, unsigned column_num,
        const char *src_start, unsigned src_len, struct Expr *lhs,
        struct Expr *rhs, enum PrimitiveType lhs_type,
        enum PrimitiveType rhs_type, u32 int_value, enum ExprType expr_type);
/* Uses the passed token for line and column numbers aswell as src_start and
 * src_len. */
struct Expr Expr_create_w_tok(struct Token token, struct Expr *lhs,
        struct Expr *rhs, enum PrimitiveType lhs_type,
        enum PrimitiveType rhs_type, u32 int_value, enum ExprType expr_type);
void Expr_free(struct Expr *self);
enum PrimitiveType Expr_type(struct Expr *self);
u32 Expr_evaluate(struct Expr *expr);

struct ExprStmt {

    struct Expr *expr;

};

struct ExprStmt ExprStmt_init(void);
struct ExprStmt ExprStmt_create(struct Expr *expr);
