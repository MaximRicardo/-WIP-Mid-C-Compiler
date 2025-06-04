#include "shunting_yard.h"
#include "ast.h"
#include "lexer.h"
#include "safe_mem.h"
#include "token.h"
#include "vector_impl.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

struct SYQueue {

    /* Array of pointers to expressions, not a 2D array */
    struct Expr **elems;
    u32 size;
    u32 capacity;

};


static m_define_VectorImpl_init(SYQueue)
static m_define_VectorImpl_free(SYQueue)
static m_define_VectorImpl_push_back(SYQueue, struct Expr*)
static m_define_VectorImpl_pop_back(SYQueue)

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

    }

}

/* Moves the operator at the top of the operator queue over to the output
 * queue */
static void move_operator_to_out_queue(struct SYQueue *output_queue,
        struct SYQueue *operator_queue) {

    /* The operator takes the second upper-most and upper-most elements on the
     * output queue as its left and right operands respectively */
    struct Expr *lhs = output_queue->elems[output_queue->size-2];
    struct Expr *rhs = output_queue->elems[output_queue->size-1];

    struct Expr *operator =
        operator_queue->elems[operator_queue->size-1];

    operator->lhs = lhs;
    operator->rhs = rhs;

    /* Remove the lhs and rhs from the queue and replace them with the
     * operator. Later on the operator can then act as an operand for the next
     * operator to be pushed to the output queue */
    SYQueue_pop_back(output_queue);
    SYQueue_pop_back(output_queue);
    SYQueue_push_back(output_queue, operator);
    SYQueue_pop_back(operator_queue);

}

static void push_operator_to_queue(struct SYQueue *output_queue,
        struct SYQueue *operator_queue, enum TokenType operator_type) {

    struct Expr *expr = safe_malloc(sizeof(*expr));
    *expr = Expr_init();
    expr->type = tok_t_to_expr_t(operator_type);

    /* If the operator o2 at the top of the stack has greater precedence than
     * the current operator o1, o2 must be moved to the output queue. Then if
     * there is another operator below o2, it now becomes the new o2 and the
     * process repeats. */
    while (operator_queue->size > 0) {
        enum TokenType o2_tok_type = expr_t_to_tok_t(
            operator_queue->elems[operator_queue->size-1]->type
            );
        unsigned o1_prec = Token_precedence(operator_type);
        unsigned o2_prec = Token_precedence(o2_tok_type);

        /* Remember, precedence levels in c are reversed, so 1 is the highest
         * level and 15 is the lowest. */
        if (!(o2_prec < o1_prec || (Token_l_to_right_asso(operator_type) &&
                 o2_prec == o1_prec)))
            break;

        assert(output_queue->size >= 2);

        move_operator_to_out_queue(output_queue, operator_queue);
    }

    SYQueue_push_back(operator_queue, expr);

}

struct Expr* shunting_yard(const struct Lexer *lexer) {

    struct SYQueue output_queue = SYQueue_init();
    struct SYQueue operator_queue = SYQueue_init();
    unsigned i;

    for (i = 0; i < lexer->token_tbl.size; i++) {

        if (Token_is_operator(lexer->token_tbl.elems[i].type)) {
            push_operator_to_queue(&output_queue, &operator_queue,
                    lexer->token_tbl.elems[i].type);
        }
        else {
            struct Expr *expr = safe_malloc(sizeof(*expr));
            *expr = Expr_create(NULL, NULL, PrimType_INT, PrimType_INVALID,
                    lexer->token_tbl.elems[i].value.int_value,
                    ExprType_INT_LIT);
            SYQueue_push_back(&output_queue, expr);
        }

    }

    while (operator_queue.size > 0) {
        move_operator_to_out_queue(&output_queue, &operator_queue);
    }
    SYQueue_free(&operator_queue);

    assert(output_queue.size == 1);
    {
        struct Expr *expr = output_queue.elems[0];
        SYQueue_free(&output_queue);
        return expr;
    }

}
