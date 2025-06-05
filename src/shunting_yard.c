#include "shunting_yard.h"
#include "ast.h"
#include "safe_mem.h"
#include "token.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

bool SY_error_occurred = false;

static enum ExprType tok_t_to_expr_t(enum TokenType type) {

    switch (type) {

    case TokenType_NONE:
        return ExprType_INVALID;

    case TokenType_PLUS:
        return ExprType_PLUS;

    case TokenType_MINUS:
        return ExprType_MINUS;

    case TokenType_MUL:
        return ExprType_MUL;

    case TokenType_DIV:
        return ExprType_DIV;

    case TokenType_INT_LIT:
        return ExprType_INT_LIT;

    case TokenType_L_PAREN:
        return ExprType_PAREN;

    default:
        assert(false);

    }

}

static enum TokenType expr_t_to_tok_t(enum ExprType type) {

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

    case ExprType_INT_LIT:
        return TokenType_INT_LIT;

    case ExprType_PAREN:
        return TokenType_L_PAREN;

    }

}

/* Moves the operator at the top of the operator queue over to the output
 * queue */
static void move_operator_to_out_queue(struct ExprPtrList *output_queue,
        struct ExprPtrList *operator_stack) {

    /* The operator takes the second upper-most and upper-most elements on the
     * output queue as its left and right operands respectively */
    struct Expr *lhs = output_queue->elems[output_queue->size-2];
    struct Expr *rhs = output_queue->elems[output_queue->size-1];

    struct Expr *operator =
        operator_stack->elems[operator_stack->size-1];

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
        struct ExprPtrList *operator_stack, enum TokenType operator_type) {

    struct Expr *expr = safe_malloc(sizeof(*expr));
    *expr = Expr_init();
    expr->expr_type = tok_t_to_expr_t(operator_type);

    /* If the operator o2 at the top of the stack has greater precedence than
     * the current operator o1, o2 must be moved to the output queue. Then if
     * there is another operator below o2, it now becomes the new o2 and the
     * process repeats. */
    while (operator_stack->size > 0 &&
            ExprPtrList_back(operator_stack)->expr_type != ExprType_PAREN) {
        enum TokenType o2_tok_type = expr_t_to_tok_t(
                ExprPtrList_back(operator_stack)->expr_type
            );
        unsigned o1_prec = Token_precedence(operator_type);
        unsigned o2_prec = Token_precedence(o2_tok_type);

        /* Remember, precedence levels in c are reversed, so 1 is the highest
         * level and 15 is the lowest. */
        if (!(o2_prec < o1_prec || (Token_l_to_right_asso(operator_type) &&
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
        enum TokenType stop_type, u32 *end_idx) {

    struct ExprPtrList output_queue = ExprPtrList_init();
    struct ExprPtrList operator_stack = ExprPtrList_init();
    unsigned i;

    SY_error_occurred = false;

    for (i = start_idx; i < token_tbl->size &&
            token_tbl->elems[i].type != stop_type; i++) {

        if (Token_is_operator(token_tbl->elems[i].type)) {
            push_operator_to_queue(&output_queue, &operator_stack,
                    token_tbl->elems[i].type);
        }
        else if (token_tbl->elems[i].type == TokenType_L_PAREN) {
            struct Expr *expr = safe_malloc(sizeof(*expr));
            *expr = Expr_create_w_tok(token_tbl->elems[i], NULL, NULL,
                    PrimType_INVALID, PrimType_INVALID, 0, ExprType_PAREN);
            ExprPtrList_push_back(&operator_stack, expr);
        }
        else if (token_tbl->elems[i].type == TokenType_R_PAREN) {
            read_r_paren(&output_queue, &operator_stack, &token_tbl->elems[i]);
        }
        else {
            struct Expr *expr = safe_malloc(sizeof(*expr));
            *expr = Expr_create_w_tok(token_tbl->elems[i], NULL, NULL,
                    PrimType_INT, PrimType_INVALID,
                    token_tbl->elems[i].value.int_value, ExprType_INT_LIT);
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
