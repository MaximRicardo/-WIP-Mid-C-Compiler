#pragma once

#include "ints.h"
#include "token.h"
#include "vector_impl.h"

enum PrimitiveType {

    PrimType_INVALID,

    PrimType_INT

};

bool PrimitiveType_signed(enum PrimitiveType type);
unsigned PrimitiveType_size(enum PrimitiveType type);
enum PrimitiveType PrimitiveType_promote(enum PrimitiveType type);

enum ASTNodeType {

    ASTType_INVALID,

    ASTType_EXPR,
    ASTType_VAR_DECL

};

struct ASTNode {

    enum ASTNodeType type;
    void *node_struct;

};

struct ASTNode ASTNode_init(void);
struct ASTNode ASTNode_create(enum ASTNodeType type, void *node_struct);
void ASTNode_free(struct ASTNode node);

struct ASTNodeList {

    struct ASTNode *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(ASTNodeList, struct ASTNode)

/* when adding a new type make sure to update:
 *  expr_t_to_tok_t, tok_t_to_expr_t
 */
enum ExprType {

    ExprType_INVALID,

    /* Literals */
    ExprType_INT_LIT,

    /* Binary operators */
    ExprType_PLUS,
    ExprType_MINUS,
    ExprType_MUL,
    ExprType_DIV,
    ExprType_MODULUS,

    /* Only used in the shunting yard algorithm */
    ExprType_PAREN,

    ExprType_IDENT

};

struct Expr {

    unsigned line_num, column_num;
    /* Region of the source code that corresponds to this node's corresponding
     * token */
    const char *src_start; unsigned src_len;

    struct Expr *lhs, *rhs;
    enum PrimitiveType lhs_type, rhs_type;

    u32 int_value;

    i32 bp_offset;

    enum ExprType expr_type;

};

struct Expr Expr_init(void);
struct Expr Expr_create(unsigned line_num, unsigned column_num,
        const char *src_start, unsigned src_len, struct Expr *lhs,
        struct Expr *rhs, enum PrimitiveType lhs_type,
        enum PrimitiveType rhs_type, u32 int_value, i32 bp_offset,
        enum ExprType expr_type);
/* Uses the passed, tok_t_to_expr_td column numbers aswell as src_start and
 * src_len. */
struct Expr Expr_create_w_tok(struct Token token, struct Expr *lhs,
        struct Expr *rhs, enum PrimitiveType lhs_type,
        enum PrimitiveType rhs_type, u32 int_value, i32 bp_offset,
        enum ExprType expr_type);
/* Also frees self */
void Expr_recur_free_w_self(struct Expr *self);
enum PrimitiveType Expr_type(const struct Expr *self);
u32 Expr_evaluate(const struct Expr *expr);

struct ExprPtrList {

    struct Expr **elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(ExprPtrList, struct Expr*)

struct ExprNode {

    struct Expr *expr;

};

struct ExprNode ExprNode_init(void);
struct ExprNode ExprNode_create(struct Expr *expr);
void ExprNode_free_w_self(struct ExprNode *self);

struct Declarator {

    struct Expr *value;
    char *ident;

};

struct Declarator Declarator_init(void);
struct Declarator Declarator_create(struct Expr *value, char *ident);
void Declarator_free(struct Declarator decl);

struct DeclList {

    struct Declarator *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(DeclList, struct Declarator)

struct VarDeclNode {

    struct DeclList decls;
    enum PrimitiveType type;

};

struct VarDeclNode VarDeclNode_init(void);
struct VarDeclNode VarDeclNode_create(struct DeclList decls,
        enum PrimitiveType type);
void VarDeclNode_free_w_self(struct VarDeclNode *self);

struct TUNode {

    struct ASTNodeList nodes;

};

struct TUNode TUNode_init(void);
struct TUNode TUNode_create(struct ASTNodeList nodes);
void TUNode_free_w_self(struct TUNode *self);

enum ExprType tok_t_to_expr_t(enum TokenType type);
enum TokenType expr_t_to_tok_t(enum ExprType type);
