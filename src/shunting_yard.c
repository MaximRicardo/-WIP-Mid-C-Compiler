#include "shunting_yard.h"
#include "ast.h"
#include "lexer.h"
#include "safe_mem.h"
#include "token.h"
#include "vector_impl.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

bool SY_error_occurred = false;

struct SYList {

    /* Array of pointers to expressions, not a 2D array */
    struct Expr **elems;
    u32 size;
    u32 capacity;

};

m_define_VectorImpl_funcs(SYList, struct Expr*)

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
static void move_operator_to_out_queue(struct SYList *output_queue,
        struct SYList *operator_stack) {

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
    SYList_pop_back(output_queue, NULL);
    SYList_pop_back(output_queue, NULL);
    SYList_push_back(output_queue, operator);
    SYList_pop_back(operator_stack, NULL);

}

/*
 * check_below_operators  - Checks whether any previous operators in the
 *    operator stack should be popped first.
 */
static void push_operator_to_queue(struct SYList *output_queue,
        struct SYList *operator_stack, enum TokenType operator_type) {

    struct Expr *expr = safe_malloc(sizeof(*expr));
    *expr = Expr_init();
    expr->expr_type = tok_t_to_expr_t(operator_type);

    /* If the operator o2 at the top of the stack has greater precedence than
     * the current operator o1, o2 must be moved to the output queue. Then if
     * there is another operator below o2, it now becomes the new o2 and the
     * process repeats. */
    while (operator_stack->size > 0 &&
            SYList_back(operator_stack)->expr_type != ExprType_PAREN) {
        enum TokenType o2_tok_type = expr_t_to_tok_t(
                SYList_back(operator_stack)->expr_type
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

    SYList_push_back(operator_stack, expr);

}

/* Reading a right parenthesis works by sending out all the operators that have
 * been inserted to the operator stack between the left parenthesis and the
 * right one in LIFO order */
static void read_r_paren(struct SYList *output_queue,
        struct SYList *operator_stack, const struct Token *r_paren_tok) {

    while (operator_stack->size > 0 &&
            SYList_back(operator_stack)->expr_type != ExprType_PAREN) {
        move_operator_to_out_queue(output_queue, operator_stack);
    }

    if (operator_stack->size == 0) {
        fprintf(stderr, "parenthesis mismatch. line %u, column %u\n",
                r_paren_tok->line_num, r_paren_tok->column_num);
    }
    else
        SYList_pop_back(operator_stack, Expr_free);

}

struct Expr* shunting_yard(const struct Token *tokens, u32 lower_bound,
        u32 upper_bound) {

    struct SYList output_queue = SYList_init();
    struct SYList operator_stack = SYList_init();
    unsigned i;

    SY_error_occurred = false;

    for (i = lower_bound; i <= upper_bound; i++) {

        if (Token_is_operator(tokens[i].type)) {
            push_operator_to_queue(&output_queue, &operator_stack,
                    tokens[i].type);
        }
        else if (tokens[i].type == TokenType_L_PAREN) {
            struct Expr *expr = safe_malloc(sizeof(*expr));
            *expr = Expr_create_w_tok(tokens[i], NULL, NULL, PrimType_INVALID,
                    PrimType_INVALID, 0, ExprType_PAREN);
            SYList_push_back(&operator_stack, expr);
        }
        else if (tokens[i].type == TokenType_R_PAREN) {
            read_r_paren(&output_queue, &operator_stack, &tokens[i]);
        }
        else {
            struct Expr *expr = safe_malloc(sizeof(*expr));
            *expr = Expr_create_w_tok(tokens[i], NULL, NULL, PrimType_INT,
                    PrimType_INVALID, tokens[i].value.int_value,
                    ExprType_INT_LIT);
            SYList_push_back(&output_queue, expr);
        }

    }

    while (operator_stack.size > 0) {
        move_operator_to_out_queue(&output_queue, &operator_stack);
    }
    SYList_free(&operator_stack);

    assert(output_queue.size == 1);
    {
        struct Expr *expr = output_queue.elems[0];
        SYList_free(&output_queue);
        return expr;
    }

}
