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

    if (!is_root && vars->elems[var_idx].type == PrimType_VOID) {
        fprintf(stderr,
                "cannot use the function '%s' in an expression, due to it"
                " being of type 'void'. line %u, column %u.\n", func_name,
                expr->line_num, expr->column_num);
        error = true;
    }

    if ((vars->elems[var_idx].void_args && expr->args.size > 0) ||
            (vars->elems[var_idx].args->size > 0 &&
            !VarDeclPtrList_equivalent_expr(vars->elems[var_idx].args,
                &expr->args))) {
        fprintf(stderr,
                "mismatching arguments for the call to '%s' on line %u,"
                " column %u.\n", func_name, expr->line_num, expr->column_num);
        error = true;
    }

    m_free(func_name);
    return error;

}

static bool verify_expr(const struct Expr *expr, const struct ParVarList *vars,
        bool is_root) {

    bool error = false;

    if (expr->lhs)
        error |= verify_expr(expr->lhs, vars, false);
    if (expr->rhs)
        error |= verify_expr(expr->rhs, vars, false);

    if (expr->expr_type == ExprType_FUNC_CALL) {
        verify_func_call(expr, vars, is_root);
    }

    return error;

}

bool Expr_verify(const struct Expr *expr, const struct ParVarList *vars) {

    return verify_expr(expr, vars, true);

}
