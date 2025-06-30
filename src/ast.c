#include "ast.h"
#include "array_lit.h"
#include "bool.h"
#include "comp_dependent/ints.h"
#include "parser_var.h"
#include "prim_type.h"
#include "safe_mem.h"
#include "token.h"
#include "type_mods.h"
#include "vector_impl.h"
#include "macros.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        else if (node.type == ASTType_RETURN)
            RetNode_free_w_self(node.node_struct);
        else if (node.type == ASTType_IF_STMT)
            IfNode_free_w_self(node.node_struct);
        else if (node.type == ASTType_WHILE_STMT)
            WhileNode_free_w_self(node.node_struct);
        else if (node.type == ASTType_FOR_STMT)
            ForNode_free_w_self(node.node_struct);

        else if (node.type == ASTType_DEBUG_RAX)
            m_free(node.node_struct);
        else
            assert(false);
    }

}

void ASTNode_get_array_lits(const struct ASTNode *self,
        struct ArrayLitList *list) {

    if (self->node_struct) {
        if (self->type == ASTType_EXPR)
            ExprNode_get_array_lits(self->node_struct, list);
        else if (self->type == ASTType_VAR_DECL)
            VarDeclNode_get_array_lits(self->node_struct, list);
        else if (self->type == ASTType_FUNC)
            FuncDeclNode_get_array_lits(self->node_struct, list);
        else if (self->type == ASTType_BLOCK)
            BlockNode_get_array_lits(self->node_struct, list);
        else if (self->type == ASTType_RETURN)
            RetNode_get_array_lits(self->node_struct, list);
        else if (self->type == ASTType_IF_STMT)
            IfNode_get_array_lits(self->node_struct, list);
        else if (self->type == ASTType_WHILE_STMT)
            WhileNode_get_array_lits(self->node_struct, list);
        else if (self->type == ASTType_FOR_STMT)
            ForNode_get_array_lits(self->node_struct, list);
    }

}

bool ExprType_is_bin_operator(enum ExprType type) {

    return Token_is_bin_operator(expr_t_to_tok_t(type));

}

bool ExprType_is_unary_operator(enum ExprType type) {

    return Token_is_unary_operator(expr_t_to_tok_t(type));

}

bool ExprType_is_operator(enum ExprType type) {

    return Token_is_operator(expr_t_to_tok_t(type));

}

bool ExprType_is_cmp_operator(enum ExprType type) {

    return Token_is_cmp_operator(expr_t_to_tok_t(type));

}

bool ExprType_is_inc_or_dec_operator(enum ExprType type) {

    return type == ExprType_PREFIX_INC || type == ExprType_PREFIX_DEC ||
        type == ExprType_POSTFIX_INC || type == ExprType_POSTFIX_DEC;

}

bool ExprType_is_valid_ptr_operation(enum ExprType type) {

    /* add comparison operators later */
    return type == ExprType_EQUAL || type == ExprType_MINUS ||
        type == ExprType_EQUAL_TO || type == ExprType_NOT_EQUAL_TO ||
        type == ExprType_L_THAN || type == ExprType_L_THAN_OR_E ||
        type == ExprType_G_THAN || type == ExprType_G_THAN_OR_E ||
        type == ExprType_MEMBER_ACCESS_PTR;

}

bool ExprType_is_valid_single_ptr_operation(enum ExprType type) {

    return type == ExprType_PLUS || type == ExprType_MINUS ||
        type == ExprType_L_ARR_SUBSCR || type == ExprType_MEMBER_ACCESS ||
        type == ExprType_MEMBER_ACCESS_PTR;

}

bool ExprType_is_valid_unary_ptr_operation(enum ExprType type) {

    return type == ExprType_REFERENCE || type == ExprType_DEREFERENCE ||
        type == ExprType_TYPECAST || ExprType_is_inc_or_dec_operator(type) ||
        type == ExprType_SIZEOF;

}

struct Expr Expr_init(void) {

    struct Expr expr;
    expr.line_num = 0;
    expr.column_num = 0;
    expr.src_start = NULL;
    expr.src_len = 0;
    expr.file_path = NULL;
    expr.lhs = NULL;
    expr.rhs = NULL;
    expr.args = ExprPtrList_init();
    expr.expr_type = ExprType_INVALID;
    expr.int_value = 0;
    expr.array_value = ArrayLit_init();
    expr.bp_offset = 0;
    expr.is_array = false;
    expr.array_len = 0;
    expr.lvls_of_indir = 0;
    expr.prim_type = PrimType_INVALID;
    expr.non_prom_prim_type = PrimType_INVALID;
    return expr;

}

struct Expr Expr_create(unsigned line_num, unsigned column_num,
        const char *src_start, unsigned src_len, const char *file_path,
        struct Expr *lhs, struct Expr *rhs, enum PrimitiveType prim_type,
        enum PrimitiveType non_prom_prim_type, u32 type_idx,
        unsigned lvls_of_indir,
        struct ExprPtrList args, u32 int_value, struct ArrayLit array_value,
        i32 bp_offset, enum ExprType expr_type, bool is_array, u32 array_len) {

    struct Expr expr;
    expr.line_num = line_num;
    expr.column_num = column_num;
    expr.src_start = src_start;
    expr.src_len = src_len;
    expr.file_path = file_path;
    expr.lhs = lhs;
    expr.rhs = rhs;
    expr.args = args;
    expr.int_value = int_value;
    expr.array_value = array_value;
    expr.bp_offset = bp_offset;
    expr.expr_type = expr_type;
    expr.is_array = is_array;
    expr.array_len = array_len;
    expr.lvls_of_indir = lvls_of_indir;
    expr.prim_type = prim_type;
    expr.non_prom_prim_type = non_prom_prim_type;
    expr.type_idx = type_idx;
    return expr;

}

struct Expr Expr_create_w_tok(struct Token token, struct Expr *lhs,
        struct Expr *rhs, enum PrimitiveType prim_type,
        enum PrimitiveType non_prom_prim_type, u32 type_idx,
        unsigned lvls_of_indir,
        struct ExprPtrList args, u32 int_value, struct ArrayLit array_value,
        i32 bp_offset, enum ExprType expr_type, bool is_array, u32 array_len) {

    return Expr_create(token.line_num, token.column_num, token.src_start,
            token.src_len, token.file_path, lhs, rhs, prim_type,
            non_prom_prim_type, type_idx, lvls_of_indir, args, int_value,
            array_value, bp_offset,
            expr_type, is_array, array_len);

}

void Expr_recur_free_w_self(struct Expr *self) {

    if (!self)
        return;

    if (self->lhs)
        Expr_recur_free_w_self(self->lhs);
    if (self->rhs)
        Expr_recur_free_w_self(self->rhs);

    while (self->args.size > 0)
        ExprPtrList_pop_back(&self->args, Expr_recur_free_w_self);
    ExprPtrList_free(&self->args);
    ArrayLit_free(&self->array_value);
    m_free(self);

}

static unsigned func_call_indir(const struct Expr *self,
        const struct ParVarList *vars) {

    char *expr_src = Expr_src(self);

    u32 var_idx = ParVarList_find_var(vars, expr_src);
    unsigned indir =
        var_idx == m_u32_max ? 0 : vars->elems[var_idx].lvls_of_indir;

    m_free(expr_src);

    return indir;

}

static unsigned regular_expr_indir(const struct Expr *self) {

    unsigned indir = self->rhs == NULL ?
        self->lhs->lvls_of_indir :
        m_max(self->lhs->lvls_of_indir, self->rhs->lvls_of_indir);

    if (self->expr_type == ExprType_DEREFERENCE ||
            self->expr_type == ExprType_L_ARR_SUBSCR) {
        assert(indir > 0);
        --indir;
    }
    else if (self->expr_type == ExprType_REFERENCE)
        ++indir;

    return indir;

}

unsigned Expr_lvls_of_indir(struct Expr *self, const struct ParVarList *vars) {

    if (self->expr_type == ExprType_TYPECAST) {
    }
    else if (self->expr_type == ExprType_SIZEOF) {
        self->lvls_of_indir = 0;
    }
    else if (self->expr_type == ExprType_MEMBER_ACCESS ||
            self->expr_type == ExprType_MEMBER_ACCESS_PTR) {
        self->lvls_of_indir = self->rhs->lvls_of_indir;
    }
    else if (self->expr_type == ExprType_FUNC_CALL) {
        self->lvls_of_indir = func_call_indir(self, vars);
    }
    else if (self->expr_type == ExprType_ARRAY_LIT) {
        /* assume the array is a string */
        self->lvls_of_indir = 1;
    }
    else {
        self->lvls_of_indir = regular_expr_indir(self);
    }

    return self->lvls_of_indir;

}

/* implements the C arithmetic type converisons */
static enum PrimitiveType bin_expr_type(struct Expr *self,
        const struct ParVarList *vars, const struct StructList *structs) {

    enum PrimitiveType t = PrimType_INVALID;

    enum PrimitiveType lhs_prom =
        PrimitiveType_promote(self->lhs->prim_type, self->lhs->lvls_of_indir);
    enum PrimitiveType rhs_prom =
        PrimitiveType_promote(self->rhs->prim_type, self->rhs->lvls_of_indir);

    u32 lhs_prom_size = PrimitiveType_size(lhs_prom,
            self->lhs->lvls_of_indir, self->lhs->type_idx, structs);
    u32 rhs_prom_size = PrimitiveType_size(rhs_prom,
            self->rhs->lvls_of_indir, self->rhs->type_idx, structs);

    if (self->rhs->lvls_of_indir > self->lhs->lvls_of_indir)
        t = rhs_prom;
    else if (self->lhs->lvls_of_indir > self->rhs->lvls_of_indir)
        t = lhs_prom;
    else {
        if (lhs_prom == rhs_prom)
            t = lhs_prom;
        else if (lhs_prom_size > rhs_prom_size)
            t = lhs_prom;
        else if (lhs_prom_size < rhs_prom_size)
            t = rhs_prom;
        /* unsigned types always "win" */
        else if (PrimitiveType_signed(lhs_prom, self->lhs->lvls_of_indir))
            t = rhs_prom;
        else
            t = lhs_prom;
    }

    /* arrays need to be dereferenced */
    if (self->expr_type == ExprType_L_ARR_SUBSCR &&
            Expr_lvls_of_indir(self, vars) == 0) {
        t = PrimitiveType_promote(t, 0);
    }

    return t;

}

/* if the function called is undefined, then it's assumed to be an int func */
static enum PrimitiveType func_call_type(const struct Expr *self,
        const struct ParVarList *vars) {

    char *expr_src = Expr_src(self);

    u32 var_idx = ParVarList_find_var(vars, expr_src);
    enum PrimitiveType t;
    unsigned indir;

    if (var_idx == m_u32_max) {
        t = PrimType_INT;
        indir = 0;
    }
    else {
        t = vars->elems[var_idx].type;
        indir = vars->elems[var_idx].lvls_of_indir;
    }

    m_free(expr_src);

    if (t != PrimType_VOID)
        t = PrimitiveType_promote(t, indir);

    return t;

}

enum PrimitiveType Expr_type(struct Expr *self,
        const struct ParVarList *vars, const struct StructList *structs) {

    if (!self->lhs && !self->rhs) {
        if (self->non_prom_prim_type == PrimType_INVALID)
            Expr_type_no_prom(self, vars);
        return PrimitiveType_promote(self->non_prom_prim_type,
                self->lvls_of_indir);
    }

    if (self->lhs)
        self->type_idx = self->lhs->type_idx;

    if (self->expr_type == ExprType_TYPECAST) {
    }
    else if (self->expr_type == ExprType_SIZEOF) {
        self->prim_type = PrimType_ULONG;
    }
    else if (self->expr_type == ExprType_MEMBER_ACCESS ||
            self->expr_type == ExprType_MEMBER_ACCESS_PTR) {
        self->type_idx = self->rhs->type_idx;
        self->prim_type = PrimitiveType_promote(self->rhs->prim_type,
                self->rhs->lvls_of_indir);
    }
    else if (self->rhs) {
        self->prim_type = bin_expr_type(self, vars, structs);
    }
    else if (self->expr_type == ExprType_FUNC_CALL) {
        self->prim_type = func_call_type(self, vars);
    }
    else if (self->expr_type == ExprType_ARRAY_LIT) {
        /* assume the array is a string */
        self->prim_type = PrimType_CHAR;
    }
    else {
        self->prim_type = PrimitiveType_promote(self->lhs->prim_type,
                    Expr_lvls_of_indir(self, vars));
    }

    if (!PrimitiveType_non_prim_type(self->prim_type))
        self->type_idx = 0;
    return self->prim_type;

}

/* if the function called is undefined, then it's assumed to be an int func */
static enum PrimitiveType func_call_type_no_prom(const struct Expr *self,
        const struct ParVarList *vars) {

    char *expr_src = Expr_src(self);

    u32 var_idx = ParVarList_find_var(vars, expr_src);
    enum PrimitiveType t =
        var_idx == m_u32_max ? PrimType_INT : vars->elems[var_idx].type;

    m_free(expr_src);

    return t;

}

enum PrimitiveType Expr_type_no_prom(struct Expr *self,
        const struct ParVarList *vars) {

    if (self->lhs)
        self->type_idx = self->lhs->type_idx;

    if (self->expr_type == ExprType_TYPECAST) {
    }
    else if (self->expr_type == ExprType_SIZEOF) {
        self->non_prom_prim_type = PrimType_ULONG;
    }
    else if (self->expr_type == ExprType_MEMBER_ACCESS ||
            self->expr_type == ExprType_MEMBER_ACCESS_PTR) {

        self->type_idx = self->rhs->type_idx;
        self->non_prom_prim_type = self->rhs->non_prom_prim_type;

    }
    else if (self->expr_type == ExprType_FUNC_CALL) {
        self->non_prom_prim_type = func_call_type_no_prom(self, vars);
    }
    else if (self->expr_type == ExprType_ARRAY_LIT) {
        /* assume the array is a string */
        self->non_prom_prim_type = PrimType_CHAR;
    }
    else if (self->rhs && self->rhs->lvls_of_indir > self->lhs->lvls_of_indir)
        self->non_prom_prim_type = self->rhs->non_prom_prim_type;
    else
        self->non_prom_prim_type = self->lhs->non_prom_prim_type;

    if (!PrimitiveType_non_prim_type(self->prim_type))
        self->type_idx = 0;
    return self->non_prom_prim_type;

}

u32 Expr_evaluate(const struct Expr *self, const struct StructList *structs) {

#define m_bin_oper(operation) \
    do { \
        if (is_signed) \
            return (i32)lhs_val operation (i32)rhs_val; \
        else \
            return lhs_val operation rhs_val; \
    } while (0)

#define m_unary_oper(operation) \
    do { \
        if (is_signed) \
            return operation (i32)lhs_val; \
        else \
            return operation lhs_val; \
    } while (0)

#define m_postf_unary_oper(operation) \
    do { \
        if (is_signed) \
            return ((i32)lhs_val) operation; \
        else \
            return lhs_val operation; \
    } while (0)

    u32 lhs_val;
    u32 rhs_val;
    bool is_signed;

    if (self->expr_type == ExprType_SIZEOF) {
        u32 mul = self->lhs->is_array ? self->lhs->array_len : 1;
        return PrimitiveType_size(self->lhs->prim_type,
                self->lhs->lvls_of_indir, self->lhs->type_idx, structs) * mul;
    }

    lhs_val = self->lhs ? Expr_evaluate(self->lhs, structs) :
        !self->rhs ? self->int_value : 0;
    rhs_val = self->rhs ? Expr_evaluate(self->rhs, structs) : 0;

    is_signed = PrimitiveType_signed(self->prim_type, self->lvls_of_indir);

    assert(!(!self->lhs && self->rhs));

    switch (self->expr_type) {

    case ExprType_PLUS:
        m_bin_oper(+);

    case ExprType_MINUS:
        m_bin_oper(-);

    case ExprType_MUL:
        m_bin_oper(*);

    case ExprType_DIV:
        m_bin_oper(/);

    case ExprType_MODULUS:
        m_bin_oper(%);

    case ExprType_COMMA:
        return 0;

    case ExprType_BITWISE_AND:
        m_bin_oper(&);

    case ExprType_BOOLEAN_OR:
        m_bin_oper(||);

    case ExprType_BOOLEAN_AND:
        m_bin_oper(&&);

    case ExprType_EQUAL_TO:
        m_bin_oper(==);

    case ExprType_NOT_EQUAL_TO:
        m_bin_oper(!=);

    case ExprType_L_THAN:
        m_bin_oper(<);

    case ExprType_L_THAN_OR_E:
        m_bin_oper(<=);

    case ExprType_G_THAN:
        m_bin_oper(>);

    case ExprType_G_THAN_OR_E:
        m_bin_oper(>=);

    case ExprType_BITWISE_NOT:
        m_unary_oper(~);

    case ExprType_BOOLEAN_NOT:
        m_unary_oper(!);

    case ExprType_POSITIVE:
        m_unary_oper(+);

    case ExprType_NEGATIVE:
        m_unary_oper(-);

    case ExprType_TYPECAST:
        return lhs_val;

    case ExprType_INT_LIT:
        return lhs_val;

    default:
        fprintf(stderr, "%s, %u,%u\n", self->file_path, self->line_num,
                self->column_num);
        assert(false);

    }

#undef m_bin_oper
#undef m_unary_oper
#undef m_postf_unary_oper

}

char* Expr_src(const struct Expr *self) {

    char *str = malloc((self->src_len+1)*sizeof(*str));
    strncpy(str, self->src_start, self->src_len);
    str[self->src_len] = '\0';

    return str;

}

void Expr_get_array_lits(const struct Expr *self, struct ArrayLitList *list) {

    u32 i;

    if (self->lhs)
        Expr_get_array_lits(self->lhs, list);
    if (self->rhs)
        Expr_get_array_lits(self->rhs, list);

    for (i = 0; i < self->args.size; i++) {
        Expr_get_array_lits(self->args.elems[i], list);
    }

    if (self->expr_type != ExprType_ARRAY_LIT)
        return;

    ArrayLitList_push_back(list, self->array_value);

}

bool Expr_statically_evaluatable(const struct Expr *self) {

    if (self->expr_type == ExprType_IDENT ||
            self->expr_type == ExprType_REFERENCE ||
            self->expr_type == ExprType_DEREFERENCE ||
            self->expr_type == ExprType_L_ARR_SUBSCR ||
            ExprType_is_inc_or_dec_operator(self->expr_type) ||
            self->expr_type == ExprType_FUNC_CALL ||
            self->expr_type == ExprType_ARRAY_LIT ||
            self->expr_type == ExprType_MEMBER_ACCESS ||
            self->expr_type == ExprType_MEMBER_ACCESS_PTR)
        return false;

    else if (self->expr_type == ExprType_SIZEOF)
        return true;

    if (self->lhs && !Expr_statically_evaluatable(self->lhs))
        return false;
    else if (self->rhs && !Expr_statically_evaluatable(self->rhs))
        return false;

    return true;

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

void ExprNode_get_array_lits(const struct ExprNode *self,
        struct ArrayLitList *list) {

    if (self->expr)
        Expr_get_array_lits(self->expr, list);

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

void BlockNode_get_array_lits(const struct BlockNode *self,
        struct ArrayLitList *list) {

    u32 i;

    for (i = 0; i < self->nodes.size; i++) {

        ASTNode_get_array_lits(&self->nodes.elems[i], list);

    }

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

    case TokenType_L_ARR_SUBSCR:
        return ExprType_L_ARR_SUBSCR;

    case TokenType_BITWISE_AND:
        return ExprType_BITWISE_AND;

    case TokenType_BOOLEAN_OR:
        return ExprType_BOOLEAN_OR;

    case TokenType_BOOLEAN_AND:
        return ExprType_BOOLEAN_AND;

    case TokenType_EQUAL_TO:
        return ExprType_EQUAL_TO;

    case TokenType_NOT_EQUAL_TO:
        return ExprType_NOT_EQUAL_TO;

    case TokenType_L_THAN:
        return ExprType_L_THAN;

    case TokenType_L_THAN_OR_E:
        return ExprType_L_THAN_OR_E;

    case TokenType_G_THAN:
        return ExprType_G_THAN;

    case TokenType_G_THAN_OR_E:
        return ExprType_G_THAN_OR_E;

    case TokenType_MEMBER_ACCESS:
        return ExprType_MEMBER_ACCESS;

    case TokenType_MEMBER_ACCESS_PTR:
        return ExprType_MEMBER_ACCESS_PTR;

    case TokenType_BITWISE_NOT:
        return ExprType_BITWISE_NOT;

    case TokenType_BOOLEAN_NOT:
        return ExprType_BOOLEAN_NOT;

    case TokenType_POSITIVE:
        return ExprType_POSITIVE;

    case TokenType_NEGATIVE:
        return ExprType_NEGATIVE;

    case TokenType_REFERENCE:
        return ExprType_REFERENCE;

    case TokenType_DEREFERENCE:
        return ExprType_DEREFERENCE;

    case TokenType_PREFIX_INC:
        return ExprType_PREFIX_INC;

    case TokenType_PREFIX_DEC:
        return ExprType_PREFIX_DEC;

    case TokenType_POSTFIX_INC:
        return ExprType_POSTFIX_INC;

    case TokenType_POSTFIX_DEC:
        return ExprType_POSTFIX_DEC;

    case TokenType_TYPECAST:
        return ExprType_TYPECAST;

    case TokenType_SIZEOF:
        return ExprType_SIZEOF;

    case TokenType_FUNC_CALL:
        return ExprType_FUNC_CALL;

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

    case ExprType_L_ARR_SUBSCR:
        return TokenType_L_ARR_SUBSCR;

    case ExprType_BITWISE_AND:
        return TokenType_BITWISE_AND;

    case ExprType_BOOLEAN_OR:
        return TokenType_BOOLEAN_OR;

    case ExprType_BOOLEAN_AND:
        return TokenType_BOOLEAN_AND;

    case ExprType_EQUAL_TO:
        return TokenType_EQUAL_TO;

    case ExprType_NOT_EQUAL_TO:
        return TokenType_NOT_EQUAL_TO;

    case ExprType_L_THAN:
        return TokenType_L_THAN;

    case ExprType_L_THAN_OR_E:
        return TokenType_L_THAN_OR_E;

    case ExprType_G_THAN:
        return TokenType_G_THAN;

    case ExprType_G_THAN_OR_E:
        return TokenType_G_THAN_OR_E;

    case ExprType_MEMBER_ACCESS:
        return TokenType_MEMBER_ACCESS;

    case ExprType_MEMBER_ACCESS_PTR:
        return TokenType_MEMBER_ACCESS_PTR;

    case ExprType_BITWISE_NOT:
        return TokenType_BITWISE_NOT;

    case ExprType_BOOLEAN_NOT:
        return TokenType_BOOLEAN_NOT;

    case ExprType_POSITIVE:
        return TokenType_POSITIVE;

    case ExprType_NEGATIVE:
        return TokenType_NEGATIVE;

    case ExprType_REFERENCE:
        return TokenType_REFERENCE;

    case ExprType_DEREFERENCE:
        return TokenType_DEREFERENCE;

    case ExprType_PREFIX_INC:
        return TokenType_PREFIX_INC;

    case ExprType_PREFIX_DEC:
        return TokenType_PREFIX_DEC;

    case ExprType_POSTFIX_INC:
        return TokenType_POSTFIX_INC;

    case ExprType_POSTFIX_DEC:
        return TokenType_POSTFIX_DEC;

    case ExprType_TYPECAST:
        return TokenType_TYPECAST;

    case ExprType_SIZEOF:
        return TokenType_SIZEOF;

    case ExprType_FUNC_CALL:
        return TokenType_FUNC_CALL;

    case ExprType_INT_LIT:
        return TokenType_INT_LIT;

    case ExprType_ARRAY_LIT:
        assert(false);

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
    decl.lvls_of_indir = 0;
    decl.is_array = false;
    decl.array_len = 0;
    decl.bp_offset = 0;
    return decl;

}

struct Declarator Declarator_create(struct Expr *value, char *ident,
        unsigned lvls_of_indir, bool is_array, u32 array_len, u32 bp_offset) {

    struct Declarator decl;
    decl.value = value;
    decl.ident = ident;
    decl.lvls_of_indir = lvls_of_indir;
    decl.is_array = is_array;
    decl.array_len = array_len;
    decl.bp_offset = bp_offset;
    return decl;

}

void Declarator_free(struct Declarator decl) {

    Expr_recur_free_w_self(decl.value);
    m_free(decl.ident);

}

void Declarator_get_array_lits(const struct Declarator *self,
        struct ArrayLitList *list) {

    if (!self->value)
        return;

    Expr_get_array_lits(self->value, list);

}

struct VarDeclNode VarDeclNode_init(void) {

    struct VarDeclNode node;
    node.decls = DeclList_init();
    node.type = PrimType_INVALID;
    node.type_idx = 0;
    node.mods = TypeModifiers_init();
    return node;

}

struct VarDeclNode VarDeclNode_create(struct DeclList decls,
        enum PrimitiveType type, u32 type_idx, struct TypeModifiers mods) {

    struct VarDeclNode node;
    node.decls = decls;
    node.type = type;
    node.type_idx = type_idx;
    node.mods = mods;
    return node;

}

void VarDeclNode_free_w_self(struct VarDeclNode *self) {

    while (self->decls.size > 0)
        DeclList_pop_back(&self->decls, Declarator_free);
    DeclList_free(&self->decls);
    m_free(self);

}

void VarDeclNode_get_array_lits(const struct VarDeclNode *self,
        struct ArrayLitList *list) {

    u32 i;

    for (i = 0; i < self->decls.size; i++) {
        Declarator_get_array_lits(&self->decls.elems[i], list);
    }

}

bool VarDeclPtrList_equivalent_expr(const struct VarDeclPtrList *self,
        const struct ExprPtrList *other, const struct ParVarList *vars,
        const struct StructList *structs, bool self_is_variadic) {

    u32 i;

    if (self_is_variadic && other->size < self->size)
        return false;
    else if (!self_is_variadic && self->size != other->size)
        return false;

    for (i = 0; i < self->size; i++) {
        u32 j;

        for (j = 0; j < self->elems[i]->decls.size; j++) {

            bool can_convert =
                PrimitiveType_can_convert_to(self->elems[i]->type,
                    self->elems[i]->decls.elems[j].lvls_of_indir,
                    self->elems[i]->type_idx,
                    other->elems[i]->prim_type,
                    other->elems[i]->lvls_of_indir,
                    other->elems[i]->type_idx);

            if (!can_convert) {
                /* print stmts for debugging */
                printf("left indir = %d, right indir = %d.\n",
                        self->elems[i]->decls.elems[j].lvls_of_indir,
                        other->elems[i]->lvls_of_indir);
                printf("other type = %d\n",
                        Expr_type(other->elems[i], vars, structs));
                return false;
            }
        }

    }

    return true;

}

struct FuncDeclNode FuncDeclNode_init(void) {

    struct FuncDeclNode func_decl;
    func_decl.args = VarDeclPtrList_init();
    func_decl.variadic_args = false;
    func_decl.void_args = false;
    func_decl.ret_lvls_of_indir = 0;
    func_decl.ret_type_mods = TypeModifiers_init();
    func_decl.ret_type = PrimType_INVALID;
    func_decl.ret_type_idx = 0;
    func_decl.body = NULL;
    func_decl.name = NULL;
    return func_decl;

}

struct FuncDeclNode FuncDeclNode_create(struct VarDeclPtrList args,
        bool variadic_args, bool void_args, unsigned ret_lvls_of_indir,
        struct TypeModifiers ret_type_mods,
        enum PrimitiveType ret_type, u32 ret_type_idx, struct BlockNode *body,
        char *name) {

    struct FuncDeclNode func_decl;
    func_decl.args = args;
    func_decl.variadic_args = variadic_args;
    func_decl.void_args = void_args;
    func_decl.ret_lvls_of_indir = ret_lvls_of_indir;
    func_decl.ret_type_mods = ret_type_mods;
    func_decl.ret_type = ret_type;
    func_decl.ret_type_idx = ret_type_idx;
    func_decl.body = body;
    func_decl.name = name;
    return func_decl;

}

void FuncDeclNode_free_w_self(struct FuncDeclNode *self) {

    while (self->args.size > 0) {
        VarDeclPtrList_pop_back(&self->args, VarDeclNode_free_w_self);
    }
    VarDeclPtrList_free(&self->args);

    m_free(self->name);
    if (self->body)
        BlockNode_free_w_self(self->body);
    m_free(self);

}

void FuncDeclNode_get_array_lits(const struct FuncDeclNode *self,
        struct ArrayLitList *list) {

    if (self->body)
        BlockNode_get_array_lits(self->body, list);

}

bool FuncDeclNode_defined(const struct FuncDeclNode *self,
        const struct BlockNode *transl_unit) {

    u32 i;

    if (self->body)
        return true;

    for (i = 0; i < transl_unit->nodes.size; i++) {

        const struct FuncDeclNode *func = NULL;

        if (transl_unit->nodes.elems[i].type != ASTType_FUNC)
            continue;

        func = transl_unit->nodes.elems[i].node_struct;

        if (!func->body)
            continue;

        if (strcmp(self->name, func->name) == 0)
            return true;

    }

    return false;

}

struct RetNode RetNode_init(void) {

    struct RetNode ret_node;
    ret_node.value = NULL;
    ret_node.lvls_of_indir = 0;
    ret_node.type = PrimType_INVALID;
    ret_node.type_idx = 0;
    ret_node.n_stack_frames_deep = 0;
    return ret_node;

}

struct RetNode RetNode_create(struct Expr *value, unsigned lvls_of_indir,
        enum PrimitiveType type, u32 type_idx, u32 n_stack_frames_deep) {

    struct RetNode ret_node;
    ret_node.value = value;
    ret_node.lvls_of_indir = lvls_of_indir;
    ret_node.type = type;
    ret_node.type_idx = type_idx;
    ret_node.n_stack_frames_deep = n_stack_frames_deep;
    return ret_node;

}

void RetNode_free_w_self(struct RetNode *self) {

    Expr_recur_free_w_self(self->value);
    m_free(self);

}

void RetNode_get_array_lits(const struct RetNode *self,
        struct ArrayLitList *list) {

    if (self->value)
        Expr_get_array_lits(self->value, list);

}

struct IfNode IfNode_init(void) {

    struct IfNode node;
    node.expr = NULL;
    node.body = NULL;
    node.else_body = NULL;
    node.body_in_block = false;
    node.else_body_in_block = false;
    return node;

}

struct IfNode IfNode_create(struct Expr *expr, struct BlockNode *body,
        struct BlockNode *else_body, bool body_in_block,
        bool else_body_in_block) {

    struct IfNode node;
    node.expr = expr;
    node.body = body;
    node.else_body = else_body;
    node.body_in_block = body_in_block;
    node.else_body_in_block = else_body_in_block;
    return node;

}

void IfNode_free_w_self(struct IfNode *self) {

    if (self->expr)
        Expr_recur_free_w_self(self->expr);
    if (self->body)
        BlockNode_free_w_self(self->body);
    if (self->else_body)
        BlockNode_free_w_self(self->else_body);
    m_free(self);

}

void IfNode_get_array_lits(const struct IfNode *self,
        struct ArrayLitList *list) {

    if (self->expr)
        Expr_get_array_lits(self->expr, list);
    if (self->body)
        BlockNode_get_array_lits(self->body, list);
    if (self->else_body)
        BlockNode_get_array_lits(self->else_body, list);

}

struct WhileNode WhileNode_init(void) {

    struct WhileNode node;
    node.expr = NULL;
    node.body = NULL;
    node.body_in_block = false;
    return node;

}

struct WhileNode WhileNode_create(struct Expr *expr, struct BlockNode *body,
        bool body_in_block) {

    struct WhileNode node;
    node.expr = expr;
    node.body = body;
    node.body_in_block = body_in_block;
    return node;

}

void WhileNode_free_w_self(struct WhileNode *self) {

    if (self->expr)
        Expr_recur_free_w_self(self->expr);
    if (self->body)
        BlockNode_free_w_self(self->body);

    m_free(self);

}

void WhileNode_get_array_lits(const struct WhileNode *self,
        struct ArrayLitList *list) {

    if (self->expr)
        Expr_get_array_lits(self->expr, list);
    if (self->body)
        BlockNode_get_array_lits(self->body, list);

}

struct ForNode ForNode_init(void) {

    struct ForNode node;
    node.init = NULL;
    node.condition = NULL;
    node.inc = NULL;
    node.body = NULL;
    node.body_in_block = false;
    return node;

}

struct ForNode ForNode_create(struct Expr *init, struct Expr *condition,
        struct Expr *inc, struct BlockNode *body, bool body_in_block) {

    struct ForNode node;
    node.init = init;
    node.condition = condition;
    node.inc = inc;
    node.body = body;
    node.body_in_block = body_in_block;
    return node;

}

void ForNode_free_w_self(struct ForNode *self) {

    if (self->init)
        Expr_recur_free_w_self(self->init);
    if (self->condition)
        Expr_recur_free_w_self(self->condition);
    if (self->inc)
        Expr_recur_free_w_self(self->inc);

    if (self->body)
        BlockNode_free_w_self(self->body);

    m_free(self);

}

void ForNode_get_array_lits(const struct ForNode *self,
        struct ArrayLitList *list) {

    if (self->init)
        Expr_get_array_lits(self->init, list);
    if (self->condition)
        Expr_get_array_lits(self->condition, list);
    if (self->inc)
        Expr_get_array_lits(self->inc, list);

    if (self->body)
        BlockNode_get_array_lits(self->body, list);

}

m_define_VectorImpl_funcs(ASTNodeList, struct ASTNode)
m_define_VectorImpl_funcs(DeclList, struct Declarator)
m_define_VectorImpl_funcs(VarDeclPtrList, struct VarDeclNode*)
m_define_VectorImpl_funcs(ExprPtrList, struct Expr*)
m_define_VectorImpl_funcs(ExprList, struct Expr)
