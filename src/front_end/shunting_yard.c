#include "shunting_yard.h"
#include "../array_lit.h"
#include "../backend_dependent/type_sizes.h"
#include "../comp_dependent/ints.h"
#include "../err_msg.h"
#include "../prim_type.h"
#include "../structs.h"
#include "../type_mods.h"
#include "../utils/macros.h"
#include "../utils/safe_mem.h"
#include "ast.h"
#include "identifier.h"
#include "parser.h"
#include "token.h"
#include "type_spec.h"
#include "typedef.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

bool SY_error_occurred = false;

/* returns m_u32_max if a token of stop_type type couldn't be found */
static u32 skip_to_token_type(u32 start_idx, struct TokenList tokens,
                              enum TokenType stop_type)
{
    u32 i;
    for (i = start_idx; i < tokens.size; i++) {
        if (tokens.elems[i].type == stop_type)
            return i;
    }
    return m_u32_max;
}

/* same as skip_to_token_type but if the token couldn't be found tokens.size-1,
 * or the index of the last token, is returned instead */
static u32 skip_to_token_type_alt(u32 start_idx, struct TokenList tokens,
                                  enum TokenType stop_type)
{
    return m_min(skip_to_token_type(start_idx, tokens, stop_type),
                 tokens.size - 1);
}

/* Moves the operator at the top of the operator queue over to the output
 * queue */
static void move_operator_to_out_queue(struct ExprPtrList *output_queue,
                                       struct ExprPtrList *operator_stack,
                                       const struct ParVarList *vars,
                                       const struct StructList *structs)
{
    struct Expr *operator= ExprPtrList_back(operator_stack);

    if (output_queue->size == 0) {
        /* an error should have already occurred by now, so no need to print
         * anything */
        assert(SY_error_occurred);
        ExprPtrList_pop_back(operator_stack, Expr_recur_free_w_self);
        return;
    }

    /* The operator takes the second upper-most and upper-most elements on the
     * output queue as its left and right operands respectively */
    operator->rhs = ExprType_is_bin_operator(operator->expr_type)
        ? output_queue->elems[output_queue->size - 1]
        : NULL;
    operator->lhs =
        output_queue->elems[output_queue->size - 1 - (operator->rhs != NULL)];

    Expr_lvls_of_indir(operator, vars);
    Expr_type(operator, vars, structs);
    Expr_type_no_prom(operator, vars);

    /* Remove the lhs and rhs from the queue and replace them with the
     * operator. Later on the operator can then act as an operand for the next
     * operator to be pushed to the output queue */
    if (operator->rhs)
        ExprPtrList_pop_back(output_queue, NULL);
    ExprPtrList_pop_back(output_queue, NULL);
    ExprPtrList_push_back(output_queue, operator);
    ExprPtrList_pop_back(operator_stack, NULL);
}

/* needs to be run before you push an operator to the stack.
 * op_tok_type          - the type of the operator you're gonna push.
 */
static void pop_higher_precedence_operators(struct ExprPtrList *output_queue,
                                            struct ExprPtrList *operator_stack,
                                            enum TokenType op_tok_type,
                                            const struct ParVarList *vars,
                                            const struct StructList *structs)
{
    /* If the operator o2 at the top of the stack has greater precedence than
     * the current operator o1, o2 must be moved to the output queue. Then if
     * there is another operator below o2, it now becomes the new o2 and the
     * process repeats. */
    while (operator_stack->size > 0 &&
           ExprPtrList_back(operator_stack)->expr_type != ExprType_PAREN) {
        enum TokenType o2_tok_type =
            expr_t_to_tok_t(ExprPtrList_back(operator_stack)->expr_type);
        unsigned o1_prec = Token_precedence(op_tok_type);
        unsigned o2_prec = Token_precedence(o2_tok_type);

        /* Remember, precedence levels in c are reversed, so 1 is the highest
         * level and 15 is the lowest. */
        if (!(o2_prec < o1_prec ||
              (Token_l_to_right_asso(op_tok_type) && o2_prec == o1_prec)))
            break;

        assert(output_queue->size >=
               (Token_is_bin_operator(o2_tok_type) ? 2 : 1));

        move_operator_to_out_queue(output_queue, operator_stack, vars, structs);
    }
}

/*
 * check_below_operators  - Checks whether any previous operators in the
 *    operator stack should be popped first.
 */
static void push_operator_to_stack(struct ExprPtrList *output_queue,
                                   struct ExprPtrList *operator_stack,
                                   struct Token op_tok,
                                   const struct ParVarList *vars,
                                   const struct StructList *structs)
{
    struct Expr *expr = safe_malloc(sizeof(*expr));
    *expr = Expr_create_w_tok(op_tok, NULL, NULL, PrimType_INVALID,
                              PrimType_INVALID, 0, 0, ExprPtrList_init(), 0,
                              ArrayLit_init(), 0, tok_t_to_expr_t(op_tok.type),
                              false, 0);

    pop_higher_precedence_operators(output_queue, operator_stack, op_tok.type,
                                    vars, structs);

    ExprPtrList_push_back(operator_stack, expr);
}

/* Reading a right parenthesis works by sending out all the operators that have
 * been inserted to the operator stack between the left parenthesis and the
 * right one in LIFO order */
static void read_r_paren(struct ExprPtrList *output_queue,
                         struct ExprPtrList *operator_stack,
                         const struct Token *r_paren_tok,
                         const struct ParVarList *vars,
                         const struct StructList *structs)
{
    while (operator_stack->size > 0 &&
           ExprPtrList_back(operator_stack)->expr_type != ExprType_PAREN) {
        move_operator_to_out_queue(output_queue, operator_stack, vars, structs);
    }

    if (operator_stack->size == 0) {
        ErrMsg_print(ErrMsg_on, &SY_error_occurred, r_paren_tok->file_path,
                     "parenthesis mismatch. line %u, column %u\n",
                     r_paren_tok->line_num, r_paren_tok->column_num);
    } else
        ExprPtrList_pop_back(operator_stack, Expr_recur_free_w_self);
}

/* returns the index of the function call's right parenthesis, or
 * token_tbl->size if there was no right parenthesis.
 *  f_call_idx  - the index of the identifier of the function call.
 * */
static u32 read_func_call(const struct TokenList *token_tbl, u32 f_call_idx,
                          u32 bp, struct ExprPtrList *output_queue,
                          const struct ParVarList *vars,
                          const struct StructList *structs,
                          const struct TypedefList *typedefs)
{
    u32 arg_start_idx = f_call_idx + 2;

    struct Expr *expr = NULL;
    char *name = Token_src(&token_tbl->elems[f_call_idx]);
    u32 var_idx = ParVarList_find_var(vars, name);
    if (var_idx == m_u32_max) {
        ErrMsg_print(ErrMsg_on, &SY_error_occurred,
                     token_tbl->elems[f_call_idx].file_path,
                     "undeclared identifier '%s'. line %u, column %u\n", name,
                     token_tbl->elems[f_call_idx].line_num,
                     token_tbl->elems[f_call_idx].column_num);
        m_free(name);
        return skip_to_token_type_alt(f_call_idx, *token_tbl,
                                      TokenType_R_PAREN);
    }
    m_free(name);

    expr = safe_malloc(sizeof(*expr));
    *expr = Expr_create_w_tok(
        token_tbl->elems[f_call_idx], NULL, NULL, vars->elems[var_idx].type,
        vars->elems[var_idx].type, vars->elems[var_idx].type_idx,
        vars->elems[var_idx].lvls_of_indir, ExprPtrList_init(), 0,
        ArrayLit_init(), 0, ExprType_FUNC_CALL, false, 0);

    /* now, we gotta parse every expression inside the parentheses */
    while (arg_start_idx < token_tbl->size &&
           token_tbl->elems[arg_start_idx].type != TokenType_R_PAREN) {
        bool old_error_occurred = SY_error_occurred;
        bool on_a_comma =
            token_tbl->elems[arg_start_idx].type == TokenType_COMMA;
        enum TokenType stop_types[] = {TokenType_R_PAREN, TokenType_COMMA};
        struct Expr *arg = SY_shunting_yard(
            token_tbl, arg_start_idx + on_a_comma, stop_types,
            sizeof(stop_types) / sizeof(stop_types[0]), &arg_start_idx, vars,
            structs, bp, false, typedefs, false);
        SY_error_occurred |= old_error_occurred;

        if (!arg)
            continue;

        ExprPtrList_push_back(&expr->args, arg);
    }

    /* the function call is done being constructed */
    ExprPtrList_push_back(output_queue, expr);

    return arg_start_idx;
}

static bool type_is_in_array(enum TokenType type, enum TokenType *arr,
                             u32 arr_len)
{
    unsigned i;
    for (i = 0; i < arr_len; i++) {
        if (type == arr[i])
            return true;
    }
    return false;
}

static void push_array_subscr_to_stack(const struct TokenList *token_tbl,
                                       struct ExprPtrList *output_queue,
                                       struct ExprPtrList *operator_stack,
                                       u32 l_arr_subscr, u32 *end_idx,
                                       const struct ParVarList *vars,
                                       const struct StructList *structs, u32 bp,
                                       const struct TypedefList *typedefs)
{
    struct Expr *expr = NULL;
    struct Expr *value = NULL;
    enum TokenType stop_types[] = {TokenType_R_ARR_SUBSCR};
    bool old_error_occurred = SY_error_occurred;

    assert(token_tbl->elems[l_arr_subscr].type == TokenType_L_ARR_SUBSCR);

    value =
        SY_shunting_yard(token_tbl, l_arr_subscr + 1, stop_types,
                         sizeof(stop_types) / sizeof(stop_types[0]), end_idx,
                         vars, structs, bp, false, typedefs, false);
    SY_error_occurred |= old_error_occurred;

    expr = safe_malloc(sizeof(*expr));
    *expr = Expr_create_w_tok(
        token_tbl->elems[l_arr_subscr], NULL, NULL, PrimType_INVALID,
        PrimType_INVALID, 0, 0, ExprPtrList_init(), 0, ArrayLit_init(), 0,
        tok_t_to_expr_t(token_tbl->elems[l_arr_subscr].type), false, 0);

    pop_higher_precedence_operators(output_queue, operator_stack,
                                    TokenType_L_ARR_SUBSCR, vars, structs);
    ExprPtrList_push_back(operator_stack, expr);
    ExprPtrList_push_back(output_queue, value);
}

static void read_array_initializer(const struct TokenList *token_tbl,
                                   struct ExprPtrList *output_queue,
                                   u32 l_curly_idx, u32 *end_idx,
                                   const struct ParVarList *vars,
                                   const struct StructList *structs, u32 bp,
                                   const struct TypedefList *typedefs)
{
    struct Expr *array_expr = NULL;
    struct ExprPtrList values = ExprPtrList_init();
    u32 value_idx = l_curly_idx + 1;

    assert(token_tbl->elems[l_curly_idx].type == TokenType_L_CURLY);

    while (value_idx < token_tbl->size &&
           token_tbl->elems[value_idx].type != TokenType_R_CURLY) {
        struct Expr *value = NULL;
        bool old_error_occurred = SY_error_occurred;
        enum TokenType stop_types[] = {TokenType_COMMA, TokenType_R_CURLY};

        if (token_tbl->elems[value_idx].type == TokenType_COMMA) {
            ++value_idx;
            continue;
        }

        value = SY_shunting_yard(token_tbl, value_idx, stop_types,
                                 sizeof(stop_types) / sizeof(stop_types[0]),
                                 &value_idx, vars, structs, bp, false, typedefs,
                                 false);

        if (!SY_error_occurred && !Expr_statically_evaluatable(value)) {
            ErrMsg_print(ErrMsg_on, &SY_error_occurred, value->file_path,
                         "array initializer elements must be statically"
                         " evaluatable. line %u, column %u\n",
                         value->line_num, value->column_num);
        }

        SY_error_occurred |= old_error_occurred;

        ExprPtrList_push_back(&values, value);
    }

    if (value_idx >= token_tbl->size) {
        ErrMsg_print(ErrMsg_on, &SY_error_occurred,
                     token_tbl->elems[l_curly_idx].file_path,
                     "missing '}' for the initializer on line %u,"
                     " column %u\n",
                     token_tbl->elems[l_curly_idx].line_num,
                     token_tbl->elems[l_curly_idx].column_num);
    }

    array_expr = safe_malloc(sizeof(*array_expr));
    *array_expr = Expr_create_w_tok(
        token_tbl->elems[l_curly_idx], NULL, NULL, PrimType_INVALID,
        PrimType_INVALID, 0, 0, ExprPtrList_init(), 0,
        ArrayLit_create(values.elems, values.size, 0), 0, ExprType_ARRAY_LIT,
        false, 0);

    Expr_lvls_of_indir(array_expr, vars);
    Expr_type(array_expr, vars, structs);
    Expr_type_no_prom(array_expr, vars);

    ExprPtrList_push_back(output_queue, array_expr);

    *end_idx = value_idx;

    /* values doesn't need to be freed cuz it's array has been passed to the
     * array literal */
}

static void read_string(const struct TokenList *token_tbl,
                        struct ExprPtrList *output_queue, u32 str_idx,
                        u32 *end_idx)
{
    struct Expr *str_expr = NULL;
    struct ExprPtrList values = ExprPtrList_init();

    const struct Token *tok = &token_tbl->elems[str_idx];

    u32 i;
    /* account for the NULL terminator */
    u32 str_len = strlen(tok->value.string) + 1;

    for (i = 0; i < str_len; i++) {
        struct Expr *value = safe_malloc(sizeof(*value));

        *value = Expr_create_w_tok(
            token_tbl->elems[i], NULL, NULL, PrimType_INT, PrimType_INVALID, 0,
            0, ExprPtrList_init(), tok->value.string[i], ArrayLit_init(), 0,
            ExprType_INT_LIT, false, 0);

        ExprPtrList_push_back(&values, value);
    }

    str_expr = safe_malloc(sizeof(*str_expr));
    *str_expr = Expr_create_w_tok(
        token_tbl->elems[str_idx], NULL, NULL, PrimType_CHAR, PrimType_CHAR, 0,
        1, ExprPtrList_init(), 0,
        ArrayLit_create(values.elems, values.size, m_TypeSize_char), 0,
        ExprType_ARRAY_LIT, false, 0);

    ExprPtrList_push_back(output_queue, str_expr);

    *end_idx = str_idx;
}

static void read_type_cast(const struct TokenList *token_tbl,
                           struct ExprPtrList *operator_stack, u32 l_paren_idx,
                           u32 *end_idx, const struct TypedefList *typedefs,
                           const struct StructList *structs)
{
    u32 type_spec_idx = l_paren_idx + 1;

    struct Expr *expr = NULL;

    enum PrimitiveType type;
    u32 type_idx;
    unsigned lvls_of_indir;
    struct TypeModifiers mods;
    *end_idx = TypeSpec_read(token_tbl, type_spec_idx, &type, &type_idx,
                             &lvls_of_indir, &mods, typedefs, structs,
                             &SY_error_occurred);

    if (mods.is_static) {
        ErrMsg_print(ErrMsg_on, &SY_error_occurred,
                     token_tbl->elems[type_spec_idx].file_path,
                     "storage specifier in type cast. line %u, column %u.",
                     token_tbl->elems[type_spec_idx].line_num,
                     token_tbl->elems[type_spec_idx].column_num);
    }

    expr = safe_malloc(sizeof(*expr));
    *expr =
        Expr_create_w_tok(token_tbl->elems[l_paren_idx], NULL, NULL, type, type,
                          type_idx, lvls_of_indir, ExprPtrList_init(), 0,
                          ArrayLit_init(), 0, ExprType_TYPECAST, false, 0);

    ExprPtrList_push_back(operator_stack, expr);

    if (*end_idx >= token_tbl->size ||
        token_tbl->elems[*end_idx].type != TokenType_R_PAREN) {
        ErrMsg_print(ErrMsg_on, &SY_error_occurred,
                     token_tbl->elems[l_paren_idx].file_path,
                     "expected a ')' to finish the typecast on line %u,"
                     " column %u.\n",
                     token_tbl->elems[l_paren_idx].line_num,
                     token_tbl->elems[l_paren_idx].column_num);
        return;
    }
}

void read_struct_field(const struct TokenList *token_tbl,
                       struct ExprPtrList *output_queue,
                       struct ExprPtrList *operator_stack, u32 ident_idx,
                       const struct ParVarList *vars,
                       const struct StructList *structs)
{
    char *name = Token_src(&token_tbl->elems[ident_idx]);

    /* the lhs of the member access should be at the back of the
     * output queue */
    struct Expr *lhs = ExprPtrList_back(output_queue);
    u32 field_idx = Struct_field_idx(&structs->elems[lhs->type_idx], name);
    if (field_idx == m_u32_max) {
        ErrMsg_print(ErrMsg_on, &SY_error_occurred,
                     token_tbl->elems[ident_idx].file_path,
                     "accessing unknown field '%s' in struct '%s' on"
                     " line %u, column %u.\n",
                     name, structs->elems[lhs->type_idx].name,
                     token_tbl->elems[ident_idx].line_num,
                     token_tbl->elems[ident_idx].column_num);
    } else {
        const struct StructField *field =
            &structs->elems[lhs->type_idx].members.elems[field_idx];

        struct Expr *expr = safe_malloc(sizeof(*expr));
        *expr = Expr_create_w_tok(token_tbl->elems[ident_idx], NULL, NULL,
                                  field->type, field->type, field->type_idx,
                                  field->lvls_of_indir, ExprPtrList_init(), 0,
                                  ArrayLit_init(), 0, ExprType_IDENT,
                                  field->is_array, field->array_len);
        ExprPtrList_push_back(output_queue, expr);

        /* the rhs of the member access operation is always the
         * identifier immediately following it */
        move_operator_to_out_queue(output_queue, operator_stack, vars, structs);
    }

    m_free(name);
}

struct Expr *SY_shunting_yard(const struct TokenList *token_tbl, u32 start_idx,
                              enum TokenType *stop_types, u32 n_stop_types,
                              u32 *end_idx, const struct ParVarList *vars,
                              const struct StructList *structs, u32 bp,
                              bool is_initializer,
                              const struct TypedefList *typedefs,
                              bool set_parser_err_occurred)
{
    struct ExprPtrList output_queue = ExprPtrList_init();
    struct ExprPtrList operator_stack = ExprPtrList_init();
    unsigned n_parens_deep = 0;
    unsigned i;

    SY_error_occurred = false;

    for (i = start_idx; i < token_tbl->size; i++) {
        if (token_tbl->elems[i].type == TokenType_SEMICOLON)
            break;
        else if (n_parens_deep == 0 &&
                 type_is_in_array(token_tbl->elems[i].type, stop_types,
                                  n_stop_types))
            break;

        if (token_tbl->elems[i].type == TokenType_L_ARR_SUBSCR) {
            push_array_subscr_to_stack(token_tbl, &output_queue,
                                       &operator_stack, i, &i, vars, structs,
                                       bp, typedefs);
        } else if (Token_is_operator(token_tbl->elems[i].type)) {
            push_operator_to_stack(&output_queue, &operator_stack,
                                   token_tbl->elems[i], vars, structs);
        } else if (token_tbl->elems[i].type == TokenType_L_PAREN) {
            /* it could be a typecast */
            char *next_src = i + 1 < token_tbl->size
                                 ? Token_src(&token_tbl->elems[i + 1])
                                 : NULL;

            bool is_type_spec =
                Ident_type_spec(next_src, typedefs) != PrimType_INVALID;
            bool is_type_mod =
                Ident_modifier_str_to_tok(next_src) != TokenType_NONE;

            if (next_src && (is_type_spec || is_type_mod)) {
                read_type_cast(token_tbl, &operator_stack, i, &i, typedefs,
                               structs);
            } else {
                struct Expr *expr = safe_malloc(sizeof(*expr));
                *expr = Expr_create_w_tok(
                    token_tbl->elems[i], NULL, NULL, PrimType_INVALID,
                    PrimType_INVALID, 0, 0, ExprPtrList_init(), 0,
                    ArrayLit_init(), 0, ExprType_PAREN, false, 0);
                ExprPtrList_push_back(&operator_stack, expr);
                ++n_parens_deep;
            }
            m_free(next_src);
        } else if (token_tbl->elems[i].type == TokenType_R_PAREN) {
            read_r_paren(&output_queue, &operator_stack, &token_tbl->elems[i],
                         vars, structs);
            --n_parens_deep;
        } else if (token_tbl->elems[i].type == TokenType_L_CURLY) {
            read_array_initializer(token_tbl, &output_queue, i, &i, vars,
                                   structs, bp, typedefs);
        } else if (token_tbl->elems[i].type == TokenType_STR_LIT) {
            read_string(token_tbl, &output_queue, i, &i);
        } else if (i + 1 < token_tbl->size &&
                   token_tbl->elems[i].type == TokenType_IDENT &&
                   token_tbl->elems[i + 1].type == TokenType_L_PAREN) {
            u32 old_i = i;
            i = read_func_call(token_tbl, i, bp, &output_queue, vars, structs,
                               typedefs);
            if (i == token_tbl->size) {
                char *func_name = Token_src(&token_tbl->elems[old_i]);
                ErrMsg_print(ErrMsg_on, &SY_error_occurred,
                             token_tbl->elems[old_i].file_path,
                             "missing ')' to finish the call to %s on line %u,"
                             " column %u\n",
                             func_name, token_tbl->elems[old_i].line_num,
                             token_tbl->elems[old_i].column_num);
                m_free(func_name);
            }
        } else if (token_tbl->elems[i].type == TokenType_IDENT) {
            struct Expr *expr = NULL;
            char *name = Token_src(&token_tbl->elems[i]);
            u32 var_idx = ParVarList_find_var(vars, name);

            if (i > 0 &&
                (token_tbl->elems[i - 1].type == TokenType_MEMBER_ACCESS_PTR ||
                 token_tbl->elems[i - 1].type == TokenType_MEMBER_ACCESS)) {
                read_struct_field(token_tbl, &output_queue, &operator_stack, i,
                                  vars, structs);
            } else if (var_idx == m_u32_max) {
                ErrMsg_print(ErrMsg_on, &SY_error_occurred,
                             token_tbl->elems[i].file_path,
                             "undeclared identifier '%s'. line %u, column %u\n",
                             name, token_tbl->elems[i].line_num,
                             token_tbl->elems[i].column_num);
            } else {
                expr = safe_malloc(sizeof(*expr));
                *expr = Expr_create_w_tok(
                    token_tbl->elems[i], NULL, NULL, vars->elems[var_idx].type,
                    vars->elems[var_idx].type, vars->elems[var_idx].type_idx,
                    vars->elems[var_idx].lvls_of_indir, ExprPtrList_init(), 0,
                    ArrayLit_init(), vars->elems[var_idx].stack_pos - bp,
                    ExprType_IDENT, vars->elems[var_idx].is_array,
                    vars->elems[var_idx].array_len);
                ExprPtrList_push_back(&output_queue, expr);
            }
            m_free(name);
        } else if (token_tbl->elems[i].type == TokenType_INT_LIT) {
            struct Expr *expr = safe_malloc(sizeof(*expr));
            *expr = Expr_create_w_tok(
                token_tbl->elems[i], NULL, NULL, PrimType_INT, PrimType_INT, 0,
                0, ExprPtrList_init(), token_tbl->elems[i].value.int_value,
                ArrayLit_init(), 0, ExprType_INT_LIT, false, 0);
            ExprPtrList_push_back(&output_queue, expr);
        } else {
            ErrMsg_print(
                true, &SY_error_occurred, token_tbl->elems[i].file_path,
                "unknown token at %u,%u\n", token_tbl->elems[i].line_num,
                token_tbl->elems[i].column_num);
            assert(false);
        }
    }

    if (end_idx)
        *end_idx = i;

    if (i == start_idx) {
        assert(output_queue.size == 0);
        return NULL;
    }

    while (operator_stack.size > 0) {
        if (operator_stack.size > 0 &&
            output_queue.size <
                (ExprType_is_bin_operator(
                     ExprPtrList_back(&operator_stack)->expr_type)
                     ? 2U
                     : 1U)) {
            ErrMsg_print(ErrMsg_on, &SY_error_occurred,
                         ExprPtrList_back(&operator_stack)->file_path,
                         "missing an operand for the operator on line %u,"
                         " column %u.\n",
                         ExprPtrList_back(&operator_stack)->line_num,
                         ExprPtrList_back(&operator_stack)->column_num);
            ExprPtrList_pop_back(&operator_stack, Expr_recur_free_w_self);
        } else
            move_operator_to_out_queue(&output_queue, &operator_stack, vars,
                                       structs);
    }
    ExprPtrList_free(&operator_stack);

    if (output_queue.size == 1) {
        struct Expr *expr = output_queue.elems[0];
        ExprPtrList_free(&output_queue);
        SY_error_occurred |= Expr_verify(expr, vars, structs, is_initializer);
        if (set_parser_err_occurred)
            Parser_error_occurred |= SY_error_occurred;

        return expr;
    } else {
        ErrMsg_print(ErrMsg_on, &SY_error_occurred,
                     token_tbl->elems[start_idx].file_path,
                     "missing %s in the expression starting at line %u,"
                     " column %u.\n",
                     output_queue.size == 0 ? "operands" : "operators",
                     token_tbl->elems[start_idx].line_num,
                     token_tbl->elems[start_idx].column_num);
        if (set_parser_err_occurred)
            Parser_error_occurred |= SY_error_occurred;

        while (output_queue.size > 0)
            ExprPtrList_pop_back(&output_queue, Expr_recur_free_w_self);
        ExprPtrList_free(&output_queue);
        return NULL;
    }
}
