#pragma once

#include "comp_dependent/ints.h"
#include "token.h"
#include "vector_impl.h"
#include "prim_type.h"
#include "parser_var.h"
#include "array_lit.h"

/* make sure to update:
 *    ASTNode_free, ASTNode_get_array_lits */
enum ASTNodeType {

    ASTType_INVALID,

    ASTType_EXPR,
    ASTType_VAR_DECL,
    ASTType_FUNC,
    ASTType_BLOCK,
    ASTType_RETURN,
    ASTType_IF_STMT,
    ASTType_WHILE_STMT,

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
void ASTNode_get_array_lits(const struct ASTNode *self,
        struct ArrayLitList *list);

struct ASTNodeList {

    struct ASTNode *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(ASTNodeList, struct ASTNode)

struct BlockNode {

    struct ASTNodeList nodes;
    /* the total number of bytes the function must allocate for its vars */
    u32 var_bytes;

};

struct BlockNode BlockNode_init(void);
struct BlockNode BlockNode_create(struct ASTNodeList nodes, u32 var_bytes);
void BlockNode_free_w_self(struct BlockNode *self);
void BlockNode_get_array_lits(const struct BlockNode *self,
        struct ArrayLitList *list);

/* when adding a new type make sure to update:
 *  tok_t_to_expr_t, expr_t_to_tok_t
 */
enum ExprType {

    ExprType_INVALID,

    /* Literals */
    ExprType_INT_LIT,
    ExprType_ARRAY_LIT,

    /* Binary operators */
    ExprType_PLUS,
    ExprType_MINUS,
    ExprType_MUL,
    ExprType_DIV,
    ExprType_MODULUS,
    ExprType_EQUAL,
    ExprType_COMMA,
    ExprType_L_ARR_SUBSCR,
    ExprType_BITWISE_AND,
    ExprType_EQUAL_TO,
    ExprType_NOT_EQUAL_TO,
    ExprType_L_THAN,
    ExprType_L_THAN_OR_E,
    ExprType_G_THAN,
    ExprType_G_THAN_OR_E,

    /* Unary operators */
    ExprType_BITWISE_NOT,
    ExprType_POSITIVE,
    ExprType_NEGATIVE,
    ExprType_REFERENCE,
    ExprType_DEREFERENCE,

    /* parentheses are only used in the shunting yard algorithm */
    ExprType_PAREN,

    ExprType_IDENT,
    ExprType_FUNC_CALL

};

bool ExprType_is_bin_operator(enum ExprType type);
bool ExprType_is_unary_operator(enum ExprType type);
bool ExprType_is_operator(enum ExprType type);
bool ExprType_is_cmp_operator(enum ExprType type);
/* is the type a valid operation on an expr with 2 ptr operands */
bool ExprType_is_valid_ptr_operation(enum ExprType type);
/* is the type a valid operation on an expr with a ptr and an integer */
bool ExprType_is_valid_single_ptr_operation(enum ExprType type);
bool ExprType_is_valid_unary_ptr_operation(enum ExprType type);

struct ExprPtrList {

    struct Expr **elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(ExprPtrList, struct Expr*)

struct ExprList {

    struct Expr *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(ExprList, struct Expr)

struct Expr {

    unsigned line_num, column_num;
    /* Region of the source code that corresponds to this node's corresponding
     * token */
    const char *src_start; unsigned src_len;

    struct Expr *lhs, *rhs;
    unsigned lhs_lvls_of_indir, rhs_lvls_of_indir;
    enum PrimitiveType lhs_type, rhs_type;
    /* doesn't factor in type promotions or lvls of indir */
    unsigned lhs_og_type, rhs_og_type;
    struct ExprPtrList args; /* used by function calls */

    bool is_array;
    u32 array_len;

    /* gets set via Expr_lvls_of_indir */
    unsigned lvls_of_indir;
    /* gets set via Expr_type */
    enum PrimitiveType prim_type;
    /* gets set via Expr_type_no_prom */
    enum PrimitiveType non_prom_prim_type;

    u32 int_value;
    struct ArrayLit array_value;

    i32 bp_offset;

    enum ExprType expr_type;

};

struct Expr Expr_init(void);
/* automatically promotes the lhs and rhs types */
struct Expr Expr_create(unsigned line_num, unsigned column_num,
        const char *src_start, unsigned src_len, struct Expr *lhs,
        struct Expr *rhs, unsigned lhs_lvls_of_indir,
        unsigned rhs_lvls_of_indir, enum PrimitiveType lhs_type, 
        enum PrimitiveType rhs_type,
        struct ExprPtrList args, u32 int_value, struct ArrayLit array_value,
        i32 bp_offset, enum ExprType expr_type, bool is_array, u32 array_len);
/* Uses the passed, tok_t_to_expr_td column numbers aswell as src_start and
 * src_len. */
struct Expr Expr_create_w_tok(struct Token token, struct Expr *lhs,
        struct Expr *rhs, unsigned lhs_lvls_of_indir,
        unsigned rhs_lvls_of_indir, enum PrimitiveType lhs_type,
        enum PrimitiveType rhs_type,
        struct ExprPtrList args, u32 int_value, struct ArrayLit array_value,
        i32 bp_offset, enum ExprType expr_type, bool is_array, u32 array_len);
/* Also frees self */
void Expr_recur_free_w_self(struct Expr *self);
unsigned Expr_lvls_of_indir(struct Expr *self, const struct ParVarList *vars);
enum PrimitiveType Expr_type(struct Expr *self,
        const struct ParVarList *vars);
/* works differently from Expr_type. uses lhs_og_type and rhs_og_type instead,
 * and inherits directly from whichever operand has the highest level of indir.
 * If both have the same, then it's inherited from the left operand by default.
 */
enum PrimitiveType Expr_type_no_prom(struct Expr *self,
        const struct ParVarList *vars);
u32 Expr_evaluate(const struct Expr *expr);
char* Expr_src(const struct Expr *expr); /* same as Token_src */
/* checks if there are any errors in the expression that the shunting yard
 * function couldn't catch */
bool Expr_verify(const struct Expr *expr, const struct ParVarList *vars,
        bool is_initializer);
void Expr_get_array_lits(const struct Expr *self, struct ArrayLitList *list);
bool Expr_statically_evaluatable(const struct Expr *self);

struct ExprNode {

    struct Expr *expr;

};

struct ExprNode ExprNode_init(void);
struct ExprNode ExprNode_create(struct Expr *expr);
void ExprNode_free_w_self(struct ExprNode *self);
void ExprNode_get_array_lits(const struct ExprNode *self,
        struct ArrayLitList *list);

struct Declarator {

    struct Expr *value;
    char *ident;
    unsigned lvls_of_indir;
    bool is_array;
    u32 array_len;
    u32 bp_offset;

};

struct Declarator Declarator_init(void);
struct Declarator Declarator_create(struct Expr *value, char *ident,
        unsigned lvls_of_indir, bool is_array, u32 array_len, u32 bp_offset);
void Declarator_free(struct Declarator decl);
void Declarator_get_array_lits(const struct Declarator *self,
        struct ArrayLitList *list);

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
void VarDeclNode_get_array_lits(const struct VarDeclNode *self,
        struct ArrayLitList *list);

struct VarDeclPtrList {

    struct VarDeclNode **elems;
    u32 size;
    u32 capacity;

};

bool VarDeclPtrList_equivalent(const struct VarDeclPtrList *self,
        const struct VarDeclPtrList *other);
bool VarDeclPtrList_equivalent_expr(const struct VarDeclPtrList *self,
        const struct ExprPtrList *other, const struct ParVarList *vars);
m_declare_VectorImpl_funcs(VarDeclPtrList, struct VarDeclNode*)

struct FuncDeclNode {

    struct VarDeclPtrList args;
    bool void_args;
    unsigned ret_lvls_of_indir;
    enum PrimitiveType ret_type;
    struct BlockNode *body;
    char *name;

};

struct FuncDeclNode FuncDeclNode_init(void);
struct FuncDeclNode FuncDeclNode_create(struct VarDeclPtrList args,
        bool void_args, unsigned ret_lvls_of_indir,
        enum PrimitiveType ret_type, struct BlockNode *body, char *name);
void FuncDeclNode_free_w_self(struct FuncDeclNode *self);
void FuncDeclNode_get_array_lits(const struct FuncDeclNode *self,
        struct ArrayLitList *list);

struct RetNode {

    struct Expr *value;
    unsigned lvls_of_indir;
    enum PrimitiveType type;
    unsigned n_stack_frames_deep;

};

struct RetNode RetNode_init(void);
struct RetNode RetNode_create(struct Expr *value, unsigned lvls_of_indir,
        enum PrimitiveType type, u32 n_stack_frames_deep);
void RetNode_free_w_self(struct RetNode *self);
void RetNode_get_array_lits(const struct RetNode *self,
        struct ArrayLitList *list);

struct IfNode {

    struct Expr *expr;
    struct BlockNode *body;
    struct BlockNode *else_body;
    /* are there curly brackets around the if and else bodies or are they just
     * single expressions */
    bool body_in_block, else_body_in_block;

};

struct IfNode IfNode_init(void);
struct IfNode IfNode_create(struct Expr *expr, struct BlockNode *body,
        struct BlockNode *else_body, bool body_in_block,
        bool else_body_in_block);
void IfNode_free_w_self(struct IfNode *self);
void IfNode_get_array_lits(const struct IfNode *self,
        struct ArrayLitList *list);

struct WhileNode {

    struct Expr *expr;
    struct BlockNode *body;

    bool body_in_block;

};

struct WhileNode WhileNode_init(void);
struct WhileNode WhileNode_create(struct Expr *expr, struct BlockNode *body,
        bool body_in_block);
void WhileNode_free_w_self(struct WhileNode *self);
void WhileNode_get_array_lits(const struct WhileNode *self,
        struct ArrayLitList *list);

struct DebugPrintRAX {

    /* empty structs are a GNU extension */
    int _ignore;

};

enum ExprType tok_t_to_expr_t(enum TokenType type);
enum TokenType expr_t_to_tok_t(enum ExprType type);
