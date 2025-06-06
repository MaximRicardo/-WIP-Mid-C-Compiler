#include "shunting_yard.h"
#include "ast.h"
#include "ints.h"
#include "safe_mem.h"
#include "token.h"
#include "parser.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

bool SY_error_occurred = false;

/* Moves the operator at the top of the operator queue over to the output
 * queue */
static void move_operator_to_out_queue(struct ExprPtrList *output_queue,
        struct ExprPtrList *operator_stack) {

    /* The operator takes the second upper-most and upper-most elements on the
     * output queue as its left and right operands respectively */
    struct Expr *lhs = output_queue->elems[output_queue->size-2];
    struct Expr *rhs = output_queue->elems[output_queue->size-1];

    struct Expr *operator =
        ExprPtrList_back(operator_stack);

    operator->lhs = lhs;
    operator->rhs = rhs;
    operator->lhs_type = Expr_type(lhs);
    operator->rhs_type = Expr_type(rhs);

    /* Remove the lhs and rhs from the queue and replace them with the
     * operator. Later on the operator can then act as an operand for the next
     * operator to be pushed to the output queue */
    ExprPtrList_pop_back(output_queue, NULL);
    ExprPtrList_pop_back(output_queue, NULL);
    ExprPtrList_push_back(output_queue, operator);
    ExprPtrList_pop_back(operator_stack, NULL);

}

/*
 * check_below_operators  - Checks whether any previous operators in the
 *    operator stack should be popped first.
 */
static void push_operator_to_queue(struct ExprPtrList *output_queue,
        struct ExprPtrList *operator_stack, struct Token op_tok) {

    struct Expr *expr = safe_malloc(sizeof(*expr));
    *expr = Expr_create_w_tok(op_tok, NULL, NULL, PrimType_INVALID,
            PrimType_INVALID, 0, 0, tok_t_to_expr_t(op_tok.type));

    /* If the operator o2 at the top of the stack has greater precedence than
     * the current operator o1, o2 must be moved to the output queue. Then if
     * there is another operator below o2, it now becomes the new o2 and the
     * process repeats. */
    while (operator_stack->size > 0 &&
            ExprPtrList_back(operator_stack)->expr_type != ExprType_PAREN) {
        enum TokenType o2_tok_type = expr_t_to_tok_t(
                ExprPtrList_back(operator_stack)->expr_type
            );
        unsigned o1_prec = Token_precedence(op_tok.type);
        unsigned o2_prec = Token_precedence(o2_tok_type);

        /* Remember, precedence levels in c are reversed, so 1 is the highest
         * level and 15 is the lowest. */
        if (!(o2_prec < o1_prec || (Token_l_to_right_asso(op_tok.type) &&
                 o2_prec == o1_prec)))
            break;

        assert(output_queue->size >= 2);

        move_operator_to_out_queue(output_queue, operator_stack);
    }

    ExprPtrList_push_back(operator_stack, expr);

}

/* Reading a right parenthesis works by sending out all the operators that have
 * been inserted to the operator stack between the left parenthesis and the
 * right one in LIFO order */
static void read_r_paren(struct ExprPtrList *output_queue,
        struct ExprPtrList *operator_stack, const struct Token *r_paren_tok) {

    while (operator_stack->size > 0 &&
            ExprPtrList_back(operator_stack)->expr_type != ExprType_PAREN) {
        move_operator_to_out_queue(output_queue, operator_stack);
    }

    if (operator_stack->size == 0) {
        fprintf(stderr, "parenthesis mismatch. line %u, column %u\n",
                r_paren_tok->line_num, r_paren_tok->column_num);
        SY_error_occurred = true;
    }
    else
        ExprPtrList_pop_back(operator_stack, Expr_recur_free_w_self);

}

struct Expr* SY_shunting_yard(const struct TokenList *token_tbl, u32 start_idx,
        enum TokenType stop_type, u32 *end_idx,
        const struct ParVarList *vars, u32 bp) {

    struct ExprPtrList output_queue = ExprPtrList_init();
    struct ExprPtrList operator_stack = ExprPtrList_init();
    unsigned i;

    SY_error_occurred = false;

    for (i = start_idx; i < token_tbl->size &&
            token_tbl->elems[i].type != stop_type; i++) {

        if (Token_is_operator(token_tbl->elems[i].type)) {
            push_operator_to_queue(&output_queue, &operator_stack,
                    token_tbl->elems[i]);
        }
        else if (token_tbl->elems[i].type == TokenType_L_PAREN) {
            struct Expr *expr = safe_malloc(sizeof(*expr));
            *expr = Expr_create_w_tok(token_tbl->elems[i], NULL, NULL,
                    PrimType_INVALID, PrimType_INVALID, 0, 0, ExprType_PAREN);
            ExprPtrList_push_back(&operator_stack, expr);
        }
        else if (token_tbl->elems[i].type == TokenType_R_PAREN) {
            read_r_paren(&output_queue, &operator_stack, &token_tbl->elems[i]);
        }
        else if (token_tbl->elems[i].type == TokenType_IDENT) {
            struct Expr *expr = safe_malloc(sizeof(*expr));
            char *name = Token_src(&token_tbl->elems[i]);
            u32 var_idx = ParVarList_find_var(vars, name);
            if (var_idx == m_u32_max) {
                fprintf(stderr,
                        "undeclared identifier '%s'. line %u, column %u\n",
                        name, token_tbl->elems[i].line_num,
                        token_tbl->elems[i].column_num);
                SY_error_occurred = true;
                m_free(name);
                m_free(expr);
                continue;
            }
            m_free(name);
            *expr = Expr_create_w_tok(token_tbl->elems[i], NULL, NULL,
                    vars->elems[var_idx].type, PrimType_INVALID, 0,
                    vars->elems[var_idx].stack_pos-bp, ExprType_IDENT);
            ExprPtrList_push_back(&output_queue, expr);
        }
        else {
            struct Expr *expr = safe_malloc(sizeof(*expr));
            *expr = Expr_create_w_tok(token_tbl->elems[i], NULL, NULL,
                    PrimType_INT, PrimType_INVALID,
                    token_tbl->elems[i].value.int_value, 0, ExprType_INT_LIT);
            ExprPtrList_push_back(&output_queue, expr);
        }

    }

    if (end_idx)
        *end_idx = i;

    if (i == start_idx)
        return NULL;

    while (operator_stack.size > 0) {
        move_operator_to_out_queue(&output_queue, &operator_stack);
    }
    ExprPtrList_free(&operator_stack);

    assert(output_queue.size == 1);
    {
        struct Expr *expr = output_queue.elems[0];
        ExprPtrList_free(&output_queue);
        return expr;
    }

}
