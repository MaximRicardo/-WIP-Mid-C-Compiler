#include "ast.h"
#include "bool.h"
#include "comp_dependent/ints.h"
#include "parser_var.h"
#include "prim_type.h"
#include "safe_mem.h"
#include <assert.h>
#include <stdio.h>

static bool verify_func_call(const struct Expr *expr,
        const struct ParVarList *vars, bool is_root) {

    bool error = false;
    char *func_name = Expr_src(expr);
    u32 var_idx = ParVarList_find_var(vars, func_name);
    assert(var_idx != m_u32_max);

    if (!is_root && vars->elems[var_idx].type == PrimType_VOID &&
            vars->elems[var_idx].lvls_of_indir == 0) {
        fprintf(stderr,
                "cannot use the function '%s' in an expression, due to it"
                " being of type 'void'. line %u, column %u.\n", func_name,
                expr->line_num, expr->column_num);
        error = true;
    }

    if ((vars->elems[var_idx].void_args && expr->args.size > 0) ||
            (vars->elems[var_idx].args->size > 0 &&
            !VarDeclPtrList_equivalent_expr(vars->elems[var_idx].args,
                &expr->args, vars))) {
        fprintf(stderr,
                "mismatching arguments for the call to '%s' on line %u,"
                " column %u.\n", func_name, expr->line_num, expr->column_num);
        error = true;
    }

    m_free(func_name);
    return error;

}

static bool verify_unary_ptr_operation(const struct Expr *expr) {

    if (!ExprType_is_valid_unary_ptr_operation(expr->expr_type)) {
        char *expr_src = Expr_src(expr);

        fprintf(stderr, "cannot perform unary operation '%s' on a pointer."
                " line %u, column %u.\n", expr_src, expr->line_num,
                expr->column_num);

        m_free(expr_src);

        return true;
    }
    else if (expr->lhs_lvls_of_indir == 1 && expr->lhs_type == PrimType_VOID) {
        char *expr_src = Expr_src(expr);

        fprintf(stderr, "cannot dereference a void pointer."
                " line %u, column %u.\n", expr->line_num, expr->column_num);

        m_free(expr_src);

        return true;
    }

    return false;

}

/* for when both operands are pointers */
static bool verify_ptr_operation(const struct Expr *expr) {

    if (!ExprType_is_valid_ptr_operation(expr->expr_type)) {
        char *expr_src = Expr_src(expr);

        fprintf(stderr, "cannot perform operation '%s' on a pointer and a"
                " pointer. line %u, column %u\n", expr_src, expr->line_num,
                expr->column_num);

        m_free(expr_src);

        return true;
    }

    return false;

}

/* for when only the left operand is a pointer */
static bool verify_single_ptr_operation(const struct Expr *expr) {

    if (!ExprType_is_valid_single_ptr_operation(expr->expr_type)) {
        char *expr_src = Expr_src(expr);

        fprintf(stderr, "cannot perform operation '%s' on a pointer and a"
                " non-pointer. line %u, column %u\n", expr_src, expr->line_num,
                expr->column_num);

        m_free(expr_src);

        return true;
    }

    return false;

}

static bool verify_expr(const struct Expr *expr, const struct ParVarList *vars,
        bool is_root, bool is_initializer) {

    bool error = false;
    u32 i;

    if (expr->lhs)
        error |= verify_expr(expr->lhs, vars, false, is_initializer);
    if (expr->rhs)
        error |= verify_expr(expr->rhs, vars, false, is_initializer);

    if (expr->expr_type == ExprType_FUNC_CALL) {
        error |= verify_func_call(expr, vars, is_root);
    }
    else if (expr->expr_type == ExprType_REFERENCE) {
        if (expr->lhs->expr_type != ExprType_IDENT &&
                /* makes sure it's not a func call */
                expr->lhs->args.size == 0 &&
                expr->lhs->expr_type != ExprType_DEREFERENCE) {
            fprintf(stderr,
                    "cannot reference an operand with no address. line %u,"
                    " column %u.\n", expr->line_num, expr->column_num);
            error = true;
        }
    }
    else if (expr->lhs_lvls_of_indir > 0 &&
            ExprType_is_unary_operator(expr->expr_type)) {
        error |= verify_unary_ptr_operation(expr);
    }
    else if (expr->expr_type == ExprType_DEREFERENCE) {
        fprintf(stderr, "can not dereference a non-pointer. line %u,"
                " column %u.\n", expr->line_num, expr->column_num);
        error = true;
    }
    else if (expr->lhs_lvls_of_indir > 0 && expr->rhs_lvls_of_indir > 0 &&
            ExprType_is_bin_operator(expr->expr_type)) {
        error |= verify_ptr_operation(expr);
    }
    else if (expr->lhs_lvls_of_indir > 0 &&
            ExprType_is_bin_operator(expr->expr_type)) {
        error |= verify_single_ptr_operation(expr);
    }
    else if (!is_initializer && expr->expr_type == ExprType_ARRAY_LIT &&
            expr->array_value.elem_size == 0) {
        /* an elem size of 0 means the array isn't a string literal */
        fprintf(stderr, "cannot use array literals outside of initializers."
                " line num = %u, column num = %u.\n", expr->line_num,
                expr->column_num);
        error = true;
    }

    for (i = 0; i < expr->args.size; i++) {
        verify_expr(expr->args.elems[i], vars, false, false);
    }

    return error;

}

bool Expr_verify(const struct Expr *expr, const struct ParVarList *vars,
        bool is_initializer) {

    return verify_expr(expr, vars, true, is_initializer);

}
