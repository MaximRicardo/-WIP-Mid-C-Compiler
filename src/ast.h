#pragma once

#include "comp_dependent/ints.h"
#include "token.h"
#include "vector_impl.h"
#include "prim_type.h"
#include "parser_var.h"

enum ASTNodeType {

    ASTType_INVALID,

    ASTType_EXPR,
    ASTType_VAR_DECL,
    ASTType_FUNC,
    ASTType_BLOCK,
    ASTType_RETURN,

    ASTType_DEBUG_RAX

};

struct ASTNode {

    unsigned line_num, column_num;
    enum ASTNodeType type;
    void *node_struct;

};

struct ASTNode ASTNode_init(void);
struct ASTNode ASTNode_create(unsigned line_num, unsigned column_num,
        enum ASTNodeType type, void *node_struct);
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
    ExprType_EQUAL,
    ExprType_COMMA,

    /* Unary operators */
    ExprType_BITWISE_NOT,

    /* parentheses are only used in the shunting yard algorithm */
    ExprType_PAREN,

    ExprType_IDENT,
    ExprType_FUNC_CALL

};

bool ExprType_is_bin_operator(enum ExprType expr);
bool ExprType_is_unary_operator(enum ExprType expr);
bool ExprType_is_operator(enum ExprType expr);

struct ExprPtrList {

    struct Expr **elems;
    u32 size;
    u32 capacity;

};

struct Expr {

    unsigned line_num, column_num;
    /* Region of the source code that corresponds to this node's corresponding
     * token */
    const char *src_start; unsigned src_len;

    struct Expr *lhs, *rhs;
    enum PrimitiveType lhs_type, rhs_type;
    enum PrimitiveType og_lhs_type;  /* the unpromoted version of lhs */
    struct ExprPtrList args; /* used by function calls */

    u32 int_value;

    i32 bp_offset;

    enum ExprType expr_type;

};

struct Expr Expr_init(void);
/* automatically promotes the lhs and rhs types */
struct Expr Expr_create(unsigned line_num, unsigned column_num,
        const char *src_start, unsigned src_len, struct Expr *lhs,
        struct Expr *rhs, enum PrimitiveType og_lhs_type, 
        enum PrimitiveType og_rhs_type, struct ExprPtrList args, u32 int_value,
        i32 bp_offset, enum ExprType expr_type);
/* Uses the passed, tok_t_to_expr_td column numbers aswell as src_start and
 * src_len. */
struct Expr Expr_create_w_tok(struct Token token, struct Expr *lhs,
        struct Expr *rhs, enum PrimitiveType lhs_type,
        enum PrimitiveType rhs_type, struct ExprPtrList args, u32 int_value,
        i32 bp_offset, enum ExprType expr_type);
/* Also frees self */
void Expr_recur_free_w_self(struct Expr *self);
/* promote is mostly set to false by assignments operators to figure out how
 * many bytes need to be assigned */
enum PrimitiveType Expr_type(const struct Expr *self, bool promote);
u32 Expr_evaluate(const struct Expr *expr);
char* Expr_src(const struct Expr *expr); /* same as Token_src */
/* checks if there are any errors in the expression that the shunting yard
 * function couldn't catch */
bool Expr_verify(const struct Expr *expr, const struct ParVarList *vars);

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
    u32 bp_offset;

};

struct Declarator Declarator_init(void);
struct Declarator Declarator_create(struct Expr *value, char *ident,
        u32 bp_offset);
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

struct VarDeclPtrList {

    struct VarDeclNode **elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(VarDeclPtrList, struct VarDeclNode*)

struct FuncDeclNode {

    struct VarDeclPtrList args;
    enum PrimitiveType ret_type;
    struct BlockNode *body;
    char *name;

};

struct FuncDeclNode FuncDeclNode_init(void);
struct FuncDeclNode FuncDeclNode_create(struct VarDeclPtrList args,
        enum PrimitiveType ret_type, struct BlockNode *body, char *name);
void FuncDeclNode_free_w_self(struct FuncDeclNode *self);

struct RetNode {

    struct Expr *value;
    enum PrimitiveType type;
    unsigned n_stack_frames_deep;

};

struct RetNode RetNode_init(void);
struct RetNode RetNode_create(struct Expr *value, enum PrimitiveType type,
        u32 n_stack_frames_deep);
void RetNode_free_w_self(struct RetNode *self);

struct BlockNode {

    struct ASTNodeList nodes;
    /* the total number of bytes the function must allocate for its vars */
    u32 var_bytes;

};

struct BlockNode BlockNode_init(void);
struct BlockNode BlockNode_create(struct ASTNodeList nodes, u32 var_bytes);
void BlockNode_free_w_self(struct BlockNode *self);

struct DebugPrintRAX {

    /* empty structs are a GNU extension */
    int _ignore;

};

enum ExprType tok_t_to_expr_t(enum TokenType type);
enum TokenType expr_t_to_tok_t(enum ExprType type);
