#include "ast.h"
#include "bool.h"
#include "safe_mem.h"
#include "token.h"
#include "type_sizes.h"
#include "vector_impl.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

bool PrimitiveType_signed(enum PrimitiveType type) {

    return type == PrimType_INT;

}

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
    node.line_num = 0;
    node.column_num = 0;
    node.type = ASTType_INVALID;
    node.node_struct = NULL;
    return node;

}

struct ASTNode ASTNode_create(unsigned line_num, unsigned column_num,
        enum ASTNodeType type, void *node_struct) {

    struct ASTNode node;
    node.line_num = line_num;
    node.column_num = column_num;
    node.type = type;
    node.node_struct = node_struct;
    return node;

}

void ASTNode_free(struct ASTNode node) {

    if (node.node_struct) {
        if (node.type == ASTType_EXPR)
            ExprNode_free_w_self(node.node_struct);
        else if (node.type == ASTType_VAR_DECL)
            VarDeclNode_free_w_self(node.node_struct);
        else if (node.type == ASTType_FUNC)
            FuncDeclNode_free_w_self(node.node_struct);
        else if (node.type == ASTType_BLOCK)
            BlockNode_free_w_self(node.node_struct);
        else if (node.type == ASTType_DEBUG_RAX)
            m_free(node.node_struct);
        else
            assert(false);
    }

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
    expr.bp_offset = 0;
    return expr;

}

struct Expr Expr_create(unsigned line_num, unsigned column_num,
        const char *src_start, unsigned src_len, struct Expr *lhs,
        struct Expr *rhs, enum PrimitiveType lhs_type,
        enum PrimitiveType rhs_type, u32 int_value, i32 bp_offset,
        enum ExprType expr_type) {

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
    expr.bp_offset = bp_offset;
    expr.expr_type = expr_type;
    return expr;

}

struct Expr Expr_create_w_tok(struct Token token, struct Expr *lhs,
        struct Expr *rhs, enum PrimitiveType lhs_type,
        enum PrimitiveType rhs_type, u32 int_value, i32 bp_offset,
        enum ExprType expr_type) {

    return Expr_create(token.line_num, token.column_num, token.src_start,
            token.src_len, lhs, rhs, lhs_type, rhs_type, int_value, bp_offset,
            expr_type);

}

void Expr_recur_free_w_self(struct Expr *self) {

    if (!self)
        return;

    if (self->lhs)
        Expr_recur_free_w_self(self->lhs);
    if (self->rhs)
        Expr_recur_free_w_self(self->rhs);

    m_free(self);

}

enum PrimitiveType Expr_type(const struct Expr *self) {

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

u32 Expr_evaluate(const struct Expr *self) {

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

struct ExprNode ExprNode_init(void) {

    struct ExprNode expr_node;
    expr_node.expr = NULL;
    return expr_node;

}

struct ExprNode ExprNode_create(struct Expr *expr) {

    struct ExprNode expr_node;
    expr_node.expr = expr;
    return expr_node;

}

void ExprNode_free_w_self(struct ExprNode *self) {

    Expr_recur_free_w_self(self->expr);
    m_free(self);

}

struct BlockNode BlockNode_init(void) {

    struct BlockNode block;
    block.nodes = ASTNodeList_init();
    block.var_bytes = 0;
    return block;

}

struct BlockNode BlockNode_create(struct ASTNodeList nodes, u32 var_bytes) {

    struct BlockNode block;
    block.nodes = nodes;
    block.var_bytes = var_bytes;
    return block;

}

void BlockNode_free_w_self(struct BlockNode *self) {

    while (self->nodes.size > 0) {
        ASTNodeList_pop_back(&self->nodes, ASTNode_free);
    }

    ASTNodeList_free(&self->nodes);
    m_free(self);

}

enum ExprType tok_t_to_expr_t(enum TokenType type) {

    switch (type) {

    case TokenType_NONE:
        return ExprType_INVALID;

    case TokenType_PLUS:
        return ExprType_PLUS;

    case TokenType_MINUS:
        return ExprType_MINUS;

    case TokenType_MUL:
        return ExprType_MUL;

    case TokenType_MODULUS:
        return ExprType_MODULUS;

    case TokenType_DIV:
        return ExprType_DIV;

    case TokenType_COMMA:
        return ExprType_COMMA;

    case TokenType_INT_LIT:
        return ExprType_INT_LIT;

    case TokenType_L_PAREN:
        return ExprType_PAREN;

    case TokenType_IDENT:
        return ExprType_IDENT;

    case TokenType_EQUAL:
        return ExprType_EQUAL;

    default:
        assert(false);

    }

}

enum TokenType expr_t_to_tok_t(enum ExprType type) {

    switch (type) {

    case ExprType_INVALID:
        return TokenType_NONE;

    case ExprType_PLUS:
        return TokenType_PLUS;

    case ExprType_MINUS:
        return TokenType_MINUS;

    case ExprType_MUL:
        return TokenType_MUL;

    case ExprType_DIV:
        return TokenType_DIV;
    
    case ExprType_MODULUS:
        return TokenType_MODULUS;

    case ExprType_COMMA:
        return TokenType_COMMA;

    case ExprType_INT_LIT:
        return TokenType_INT_LIT;

    case ExprType_PAREN:
        return TokenType_L_PAREN;

    case ExprType_IDENT:
        return TokenType_IDENT;

    case ExprType_EQUAL:
        return TokenType_EQUAL;

    }

}

struct Declarator Declarator_init(void) {

    struct Declarator decl;
    decl.value = NULL;
    decl.ident = NULL;
    decl.bp_offset = 0;
    return decl;

}

struct Declarator Declarator_create(struct Expr *value, char *ident,
        u32 bp_offset) {

    struct Declarator decl;
    decl.value = value;
    decl.ident = ident;
    decl.bp_offset = bp_offset;
    return decl;

}

void Declarator_free(struct Declarator decl) {

    Expr_recur_free_w_self(decl.value);
    m_free(decl.ident);

}

struct VarDeclNode VarDeclNode_init(void) {

    struct VarDeclNode node;
    node.decls = DeclList_init();
    node.type = PrimType_INVALID;
    return node;

}

struct VarDeclNode VarDeclNode_create(struct DeclList decls,
        enum PrimitiveType type) {

    struct VarDeclNode node;
    node.decls = decls;
    node.type = type;
    return node;

}

void VarDeclNode_free_w_self(struct VarDeclNode *self) {

    while (self->decls.size > 0)
        DeclList_pop_back(&self->decls, Declarator_free);
    DeclList_free(&self->decls);
    m_free(self);

}

struct FuncDeclNode FuncDeclNode_init(void) {

    struct FuncDeclNode func_decl;
    func_decl.args = VarDeclPtrList_init();
    func_decl.ret_type = PrimType_INVALID;
    func_decl.body = NULL;
    func_decl.name = NULL;
    return func_decl;

}

struct FuncDeclNode FuncDeclNode_create(struct VarDeclPtrList args,
        enum PrimitiveType ret_type, struct BlockNode *body, char *name) {

    struct FuncDeclNode func_decl;
    func_decl.args = args;
    func_decl.ret_type = ret_type;
    func_decl.body = body;
    func_decl.name = name;
    return func_decl;

}

void FuncDeclNode_free_w_self(struct FuncDeclNode *self) {

    unsigned i;
    for (i = 0; i < self->args.size; i++) {
        VarDeclPtrList_pop_back(&self->args, VarDeclNode_free_w_self);
    }
    VarDeclPtrList_free(&self->args);

    m_free(self->name);
    if (self->body)
        BlockNode_free_w_self(self->body);
    m_free(self);

}

m_define_VectorImpl_funcs(ASTNodeList, struct ASTNode)
m_define_VectorImpl_funcs(DeclList, struct Declarator)
m_define_VectorImpl_funcs(VarDeclPtrList, struct VarDeclNode*)
m_define_VectorImpl_funcs(ExprPtrList, struct Expr*)
