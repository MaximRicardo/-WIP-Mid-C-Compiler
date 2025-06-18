#include "parser.h"
#include "ast.h"
#include "comp_dependent/ints.h"
#include "lexer.h"
#include "prim_type.h"
#include "safe_mem.h"
#include "shunting_yard.h"
#include "identifier.h"
#include "token.h"
#include "backend_dependent/type_sizes.h"
#include "macros.h"
#include "parser_var.h"
#include "typedef.h"
#include "type_spec.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

bool Parser_error_occurred = false;
struct ParVarList vars;
struct TypedefList typedefs;

static struct BlockNode* parse(const struct Lexer *lexer,
        struct FuncDeclNode *parent_func, u32 bp, u32 sp, u32 block_start_idx,
        u32 *end_idx, unsigned n_blocks_deep, bool *missing_r_curly,
        bool detect_missing_curly, u32 n_instr_to_parse);

static u32 round_down(u32 num, u32 multiple) {

    return (num/multiple)*multiple;

}

static u32 round_up(u32 num, u32 multiple) {

    u32 remainder;

    if (multiple == 0)
        return num;

    remainder = num % multiple;
    if (remainder == 0)
        return num;

    return num+multiple-remainder;

}

/* returns m_u32_max if a token of stop_type type couldn't be found */
static u32 skip_to_token_type(u32 start_idx, struct TokenList tokens,
        enum TokenType stop_type) {

    u32 i;
    for (i = start_idx; i < tokens.size; i++) {
        if (tokens.elems[i].type == stop_type)
            return i;
    }
    return m_u32_max;

}

/* same as skip_to_token_type but if the token couldn't be found tokens.size-1
 * -- the index of the last token -- is returned instead */
static u32 skip_to_token_type_alt(u32 start_idx, struct TokenList tokens,
        enum TokenType stop_type) {

    return m_min(skip_to_token_type(start_idx, tokens, stop_type),
            tokens.size-1);

}

static u32 skip_to_token_type_arr(u32 start_idx, struct TokenList tokens,
        enum TokenType *stop_types, u32 n_stop_types) {

    u32 i;
    for (i = start_idx; i < tokens.size; i++) {
        u32 j;
        for (j = 0; j < n_stop_types; j++) {
            if (tokens.elems[i].type == stop_types[j])
                return i;
        }
    }
    return m_u32_max;

}

static u32 skip_to_token_type_alt_arr(u32 start_idx, struct TokenList tokens,
        enum TokenType *stop_types, u32 n_stop_types) {

    return m_min(skip_to_token_type_arr(start_idx, tokens, stop_types,
                n_stop_types), tokens.size-1);

}

static void check_if_missing_r_curly(const struct Lexer *lexer,
        u32 block_start_idx, u32 block_end_idx, bool check_if_reached_end,
        bool *missing_r_curly) {

    if (lexer->token_tbl.elems[block_end_idx].type != TokenType_R_CURLY ||
            (check_if_reached_end &&
             block_end_idx+1 == lexer->token_tbl.size)) {
        if (missing_r_curly)
            *missing_r_curly = true;
        fprintf(stderr,
                "missing a '}' to go with the '{' on line %u, column %u.\n",
                lexer->token_tbl.elems[block_start_idx-1].line_num,
                lexer->token_tbl.elems[block_start_idx-1].column_num);
        Parser_error_occurred = true;
    }
    else if (missing_r_curly)
        *missing_r_curly = false;

}

static struct Expr* parse_expr(const struct Lexer *lexer, u32 start_idx,
        u32 *sy_end_idx, u32 bp) {

    struct Expr *expr = SY_shunting_yard(&lexer->token_tbl, start_idx, NULL, 0,
            sy_end_idx, &vars, bp, false, &typedefs);
    Parser_error_occurred |= SY_error_occurred;

    if (*sy_end_idx == lexer->token_tbl.size) {
        fprintf(stderr, "missing semicolon. line %u.\n",
                lexer->token_tbl.elems[start_idx].line_num);
        Parser_error_occurred = true;
    }

    return expr;

}

static struct Expr* var_decl_value(const struct Lexer *lexer, u32 ident_idx,
        u32 equal_sign_idx, u32 *semicolon_idx, u32 bp) {

    struct Expr *expr = NULL;

    if (equal_sign_idx+1 >= lexer->token_tbl.size ||
            lexer->token_tbl.elems[equal_sign_idx].type ==
            TokenType_SEMICOLON) {
        *semicolon_idx = equal_sign_idx;
        return NULL;
    }

    if (lexer->token_tbl.elems[equal_sign_idx].type != TokenType_EQUAL) {
        fprintf(stderr, "missing an equals sign. line %u.\n",
                lexer->token_tbl.elems[ident_idx].line_num);
        Parser_error_occurred = true;
        *semicolon_idx = ident_idx;
        while (lexer->token_tbl.elems[*semicolon_idx].type !=
                TokenType_SEMICOLON)
            ++*semicolon_idx;
        return NULL;
    }

    expr = SY_shunting_yard(&lexer->token_tbl, equal_sign_idx+1, NULL, 0,
            semicolon_idx, &vars, bp, true, &typedefs);
    Parser_error_occurred |= SY_error_occurred;

    return expr;

}

/*
 * n_func_param_bytes   - how many bytes of parameters/arguments does the func
 *                        have thus far? can be NULL if is_func_param is false.
 * sp                   - can be NULL if is_func_param is true.
 */
static struct VarDeclNode* parse_var_decl(const struct Lexer *lexer,
        u32 v_decl_idx, u32 *end_idx, u32 bp, u32 *sp, bool is_func_param,
        unsigned *n_func_param_bytes, void *par_var_parent) {

    unsigned var_size;

    struct VarDeclNode *var_decl = NULL;
    struct Expr *expr = NULL;
    struct Declarator decl;

    bool is_array = false;
    u32 array_len = 0;
    bool len_defined = true;

    enum PrimitiveType var_type;
    u32 n_lvls_of_indir;
    u32 ident_idx = read_type_spec(&lexer->token_tbl, v_decl_idx, &var_type,
            &n_lvls_of_indir, &typedefs, &Parser_error_occurred);

    if (ident_idx >= lexer->token_tbl.size ||
            lexer->token_tbl.elems[ident_idx].type != TokenType_IDENT) {
        enum TokenType stop_types[] =
            {TokenType_SEMICOLON, TokenType_COMMA, TokenType_R_PAREN};
        fprintf(stderr, "unnamed variables are not supported. line %u,"
                " column %u\n", lexer->token_tbl.elems[v_decl_idx].line_num,
                lexer->token_tbl.elems[v_decl_idx].column_num);
        Parser_error_occurred = true;
        *end_idx = skip_to_token_type_alt_arr(v_decl_idx, lexer->token_tbl,
                stop_types, sizeof(stop_types)/sizeof(stop_types[0]));
        return NULL;
    }
    else if (var_type == PrimType_VOID && n_lvls_of_indir == 0) {
        char *var_name = Token_src(&lexer->token_tbl.elems[ident_idx]);
        fprintf(stderr, "variable '%s' of type 'void' on line %u,"
                " column %u.\n", var_name,
                lexer->token_tbl.elems[v_decl_idx].line_num,
                lexer->token_tbl.elems[v_decl_idx].column_num);
        Parser_error_occurred = true;
        m_free(var_name);
        *end_idx = skip_to_token_type_alt(v_decl_idx, lexer->token_tbl,
                TokenType_SEMICOLON);
        return NULL;
    }

    is_array = ident_idx+1 < lexer->token_tbl.size &&
        lexer->token_tbl.elems[ident_idx+1].type == TokenType_L_ARR_SUBSCR;
    if (is_array) {
        enum TokenType stop_types[] = {TokenType_R_ARR_SUBSCR};
        struct Expr *len_expr = SY_shunting_yard(&lexer->token_tbl,
                ident_idx+2, stop_types,
                sizeof(stop_types)/sizeof(stop_types[0]), end_idx, &vars, bp,
                false, &typedefs);
        Parser_error_occurred |= SY_error_occurred;
        ++*end_idx;
        if (len_expr && !Expr_statically_evaluatable(len_expr)) {
            char *var_name = Token_src(&lexer->token_tbl.elems[ident_idx]);
            fprintf(stderr, "array '%s' must have a statically evaluatable"
                    " length. line %u\n", var_name,
                    lexer->token_tbl.elems[ident_idx].line_num);
            Parser_error_occurred = true;
            m_free(var_name);
            array_len = 1;
        }
        else if (!len_expr) {
            array_len = 1;
            len_defined = false;
        }
        else
            array_len = Expr_evaluate(len_expr);
        Expr_recur_free_w_self(len_expr);
        ++n_lvls_of_indir;  /* arrays act a lot like a level of pointers */
    }
    else {
        *end_idx = ident_idx+1;
    }

    if (is_array && array_len == 0) {
        char *var_name = Token_src(&lexer->token_tbl.elems[ident_idx]);
        fprintf(stderr, "array '%s' cannot have a length of 0. line %u\n",
                var_name, lexer->token_tbl.elems[ident_idx].line_num);
        Parser_error_occurred = true;
        m_free(var_name);
        array_len = 1;
    }

    if (is_array && len_defined)
        var_size = PrimitiveType_size(var_type, n_lvls_of_indir-1)*array_len;
    else if (!is_array)
        var_size = PrimitiveType_size(var_type, n_lvls_of_indir);
    if (is_func_param)
        var_size = round_up(var_size, m_TypeSize_stack_arg_min_size);
    /* var size not defined yet if this variable is an array that hasn't been
     * given a length */

    expr = is_func_param ? NULL :
        var_decl_value(lexer, ident_idx, *end_idx, end_idx, bp);
    decl = Declarator_create(expr,
            Token_src(&lexer->token_tbl.elems[ident_idx]), n_lvls_of_indir,
            is_array, array_len, 0);

    if (decl.is_array && decl.value &&
            decl.value->expr_type != ExprType_ARRAY_LIT) {
        char *var_name = Token_src(&lexer->token_tbl.elems[ident_idx]);
        fprintf(stderr, "'%s' can only be initialized by an array initializer."
                " line %u, column %u\n", var_name,
                lexer->token_tbl.elems[ident_idx].line_num,
                lexer->token_tbl.elems[ident_idx].column_num);
        Parser_error_occurred = true;
        m_free(var_name);
    }
    else if (decl.is_array && decl.value) {
        if (!len_defined) {
            array_len = decl.value->array_value.n_values;
            var_size =
                PrimitiveType_size(var_type, n_lvls_of_indir-1)*array_len;
            decl.array_len = array_len;
        }
        decl.value->array_value.elem_size = var_size/array_len;
    }
    else if (decl.is_array && !len_defined) {
        char *var_name = Token_src(&lexer->token_tbl.elems[ident_idx]);
        fprintf(stderr, "array '%s' hasn't been given a length. line %u,"
                " column %u.\n", var_name,
                lexer->token_tbl.elems[ident_idx].line_num,
                lexer->token_tbl.elems[ident_idx].column_num);
        Parser_error_occurred = true;
        m_free(var_name);
    }

    /* align the variable to its size */
    if (!is_func_param) {
        *sp = round_down(*sp, var_size);
        decl.bp_offset = *sp-bp-var_size;
    }
    else {
        *n_func_param_bytes = round_up(*n_func_param_bytes, var_size);
        decl.bp_offset = m_TypeSize_callee_saved_regs_stack_size +
            m_TypeSize_return_address_size + m_TypeSize_stack_frame_size +
            *n_func_param_bytes;
        *end_idx = ident_idx+1;
    }

    var_decl = safe_malloc(sizeof(*var_decl));
    *var_decl = VarDeclNode_init();
    var_decl->type = var_type;
    DeclList_push_back(&var_decl->decls, decl);

    {
        char *var_name = Token_src(&lexer->token_tbl.elems[ident_idx]);
        u32 prev_decl_idx = ParVarList_find_var(&vars, var_name);
        if (prev_decl_idx != m_u32_max &&
                vars.elems[prev_decl_idx].parent == par_var_parent) {
            fprintf(stderr, "variable '%s' redeclared on line %u.\n", var_name,
                    lexer->token_tbl.elems[v_decl_idx].line_num);
            Parser_error_occurred = true;
        }
        m_free(var_name);
    }

    ParVarList_push_back(&vars, ParserVar_create(
                lexer->token_tbl.elems[v_decl_idx].line_num,
                lexer->token_tbl.elems[v_decl_idx].column_num,
                Token_src(&lexer->token_tbl.elems[ident_idx]), n_lvls_of_indir,
                var_type, is_array, array_len, decl.bp_offset+bp, NULL, false,
                false, false, is_func_param, par_var_parent));
    if (!is_func_param)
        *sp -= var_size;
    else
        *n_func_param_bytes += var_size;

    if (!(lexer->token_tbl.elems[*end_idx].type == TokenType_SEMICOLON ||
            (is_func_param &&
             (lexer->token_tbl.elems[*end_idx].type == TokenType_COMMA ||
              lexer->token_tbl.elems[*end_idx].type == TokenType_R_PAREN)))) {
        fprintf(stderr, "missing '%c'. line %u.\n",
                is_func_param ? ')' : ';',
                lexer->token_tbl.elems[v_decl_idx].line_num);
        Parser_error_occurred = true;
    }

    return var_decl;

}

static bool func_prototypes_match(const struct Lexer *lexer,
        u32 prev_func_decl_var_idx, struct FuncDeclNode *func,
        u32 f_ident_tok_idx) {

    char *func_name = Token_src(&lexer->token_tbl.elems[f_ident_tok_idx]);
    u32 i;
    bool not_matching = false;
    not_matching |= vars.elems[prev_func_decl_var_idx].lvls_of_indir == 0 &&
        func->ret_lvls_of_indir > 0;
    not_matching |= vars.elems[prev_func_decl_var_idx].lvls_of_indir > 0 &&
        func->ret_lvls_of_indir == 0;
    not_matching |= vars.elems[prev_func_decl_var_idx].type != func->ret_type;
    not_matching |= vars.elems[prev_func_decl_var_idx].args->size !=
        func->args.size;
    not_matching |= vars.elems[prev_func_decl_var_idx].variadic_args !=
        func->variadic_args;
    not_matching |= vars.elems[prev_func_decl_var_idx].void_args !=
        func->void_args;
    for (i = 0; !not_matching && i < func->args.size; i++) {
        if (func->args.elems[i]->type !=
                vars.elems[prev_func_decl_var_idx].args->elems[i]->type) {
            not_matching = true;
            break;
        }
    }

    m_free(func_name);
    return !not_matching;

}

static bool is_unnamed_void_var(const struct Lexer *lexer, u32 type_spec_idx) {

    char *type_spec_src = Token_src(&lexer->token_tbl.elems[type_spec_idx]);

    bool is_true = (Ident_type_spec(type_spec_src, &typedefs) == PrimType_VOID
            && (type_spec_idx+1 >= lexer->token_tbl.size ||
                (lexer->token_tbl.elems[type_spec_idx+1].type !=
                 TokenType_IDENT &&
                 lexer->token_tbl.elems[type_spec_idx+1].type !=
                 TokenType_MUL)));

    m_free(type_spec_src);
    return is_true;

}

/* returns the right parenthesis of after the function arguments */
static u32 parse_func_args(const struct Lexer *lexer, u32 arg_decl_start_idx,
        struct VarDeclPtrList *args, bool *void_args, u32 bp,
        struct FuncDeclNode *func_node, bool *variadic_args) {

    u32 arg_decl_idx = arg_decl_start_idx;
    u32 arg_decl_end_idx = arg_decl_idx;
    u32 n_func_param_bytes = 0;

    *variadic_args = false;

    /* if the function arguments start with an unnamed void variable, that
     * means the function takes no arguments */
    if (is_unnamed_void_var(lexer, arg_decl_idx)) {
        *void_args = true;
        arg_decl_end_idx = arg_decl_idx+1;
        if (arg_decl_end_idx >= lexer->token_tbl.size ||
                lexer->token_tbl.elems[arg_decl_end_idx].type !=
                TokenType_R_PAREN) {
            return m_u32_max;
        }
        else {
            return arg_decl_end_idx;
        }
    }
    else {
        *void_args = false;
    }

    while (arg_decl_end_idx < lexer->token_tbl.size &&
            lexer->token_tbl.elems[arg_decl_end_idx].type !=
            TokenType_R_PAREN) {

        char *type_spec_src = NULL;
        struct VarDeclNode *arg;

        if (lexer->token_tbl.elems[arg_decl_idx].type == TokenType_VARIADIC) {
            *variadic_args = true;
            arg_decl_end_idx = arg_decl_idx+1;
            break;
        }

        type_spec_src = Token_src(&lexer->token_tbl.elems[arg_decl_idx]);

        if (Ident_type_spec(type_spec_src, &typedefs) == PrimType_INVALID) {
            enum TokenType stop_types[] =
                {TokenType_R_PAREN, TokenType_L_CURLY};

            fprintf(stderr,
                    "missing type specifier for '%s'. line %u, column %u\n",
                    type_spec_src,
                    lexer->token_tbl.elems[arg_decl_idx].line_num,
                    lexer->token_tbl.elems[arg_decl_idx].column_num);
            Parser_error_occurred = true;
            m_free(type_spec_src);
            arg_decl_end_idx = skip_to_token_type_alt_arr(arg_decl_idx,
                    lexer->token_tbl, stop_types,
                    sizeof(stop_types)/sizeof(stop_types[0])) - 1;
        }

        arg = parse_var_decl(lexer, arg_decl_idx, &arg_decl_end_idx, bp, NULL,
                true, &n_func_param_bytes, func_node);

        if (arg) {
            VarDeclPtrList_push_back(args, arg);
        }

        if (lexer->token_tbl.elems[arg_decl_end_idx].type == TokenType_R_PAREN) {
            m_free(type_spec_src);
            break;
        }

        if (lexer->token_tbl.elems[arg_decl_end_idx].type != TokenType_COMMA) {
            char *var_name =
                Token_src(&lexer->token_tbl.elems[arg_decl_idx+1]);
            fprintf(stderr, "expected a comma after '%s' argument declaration."
                    " line %u.\n", var_name,
                    lexer->token_tbl.elems[arg_decl_idx+1].line_num);
            Parser_error_occurred = true;
            m_free(var_name);
            m_free(type_spec_src);

            arg_decl_end_idx = skip_to_token_type_alt(arg_decl_end_idx-1,
                    lexer->token_tbl, TokenType_L_CURLY);
            --arg_decl_end_idx;

            break;
        }

        m_free(type_spec_src);
        arg_decl_idx = arg_decl_end_idx+1;

    }

    return arg_decl_end_idx;

}

static void parse_func_decl(const struct Lexer *lexer, struct BlockNode *block,
        u32 f_decl_idx, u32 *end_idx, u32 bp) {

    struct FuncDeclNode *func = safe_malloc(sizeof(*func));
    struct VarDeclPtrList args = VarDeclPtrList_init();
    bool variadic_args = false;
    bool void_args = false;
    u32 old_vars_size = vars.size;
    u32 args_end_idx;
    char *func_name = NULL;
    u32 prev_func_decl_var_idx;

    enum PrimitiveType func_type;
    u32 func_lvls_of_indir;
    u32 f_ident_idx = read_type_spec(&lexer->token_tbl, f_decl_idx, &func_type,
            &func_lvls_of_indir, &typedefs, &Parser_error_occurred);

    func_name = Token_src(&lexer->token_tbl.elems[f_ident_idx]);

    prev_func_decl_var_idx = ParVarList_find_var(&vars, func_name);

    if (prev_func_decl_var_idx == m_u32_max) { 
        /* has no earlier declaration */
        ParVarList_push_back(&vars, ParserVar_create(
                    lexer->token_tbl.elems[f_decl_idx].line_num,
                    lexer->token_tbl.elems[f_decl_idx].column_num,
                    Token_src(&lexer->token_tbl.elems[f_ident_idx]),
                    func_lvls_of_indir, func_type, false, 0, 0, &func->args,
                    false, false, false, false, block));
        ++old_vars_size;
    }

    args_end_idx = parse_func_args(lexer, f_ident_idx+2, &args, &void_args,
            bp, func, &variadic_args);
    if (prev_func_decl_var_idx == m_u32_max) {
        vars.elems[old_vars_size-1].variadic_args = variadic_args;
        vars.elems[old_vars_size-1].void_args = void_args;
    }

    if (args_end_idx == m_u32_max) {
        fprintf(stderr,
                "expected ')' to finish the list of arguments for '%s'."
                " line %u\n", func_name,
                lexer->token_tbl.elems[f_decl_idx].line_num);
        Parser_error_occurred = true;
        m_free(func_name);
        m_free(func);
        *end_idx = skip_to_token_type_alt(f_decl_idx, lexer->token_tbl,
                TokenType_SEMICOLON);
        return;
    }

    *func = FuncDeclNode_create(args, variadic_args, void_args,
            func_lvls_of_indir, func_type, NULL,
            Token_src(&lexer->token_tbl.elems[f_ident_idx]));

    if (prev_func_decl_var_idx != m_u32_max && !func_prototypes_match(lexer,
                prev_func_decl_var_idx, func, f_ident_idx)) {
        fprintf(stderr, "function '%s' declaration on line %u, column %u,"
                " does not match previous declaration on line %u,"
                " column %u.\n", func_name,
                lexer->token_tbl.elems[f_decl_idx].line_num,
                lexer->token_tbl.elems[f_decl_idx].column_num,
                vars.elems[prev_func_decl_var_idx].line_num,
                vars.elems[prev_func_decl_var_idx].column_num);
        Parser_error_occurred = true;
    }

    if (args_end_idx+1 < lexer->token_tbl.size &&
            lexer->token_tbl.elems[args_end_idx+1].type == TokenType_L_CURLY) {

        u32 func_end_idx;
        bool missing_r_curly;

        if (prev_func_decl_var_idx == m_u32_max) {
            vars.elems[old_vars_size-1].has_been_defined = true;
        }
        else if (vars.elems[prev_func_decl_var_idx].has_been_defined) {
            fprintf(stderr,
                    "function '%s' has multiple definitions. first on line %u,"
                    " then later on line %u.\n",
                    func_name, vars.elems[prev_func_decl_var_idx].line_num,
                    lexer->token_tbl.elems[f_decl_idx].line_num);
            Parser_error_occurred = false;
        }

        func->body = parse(lexer, func, bp, bp, args_end_idx+2, &func_end_idx,
                1, &missing_r_curly, true, 0);
        if (!missing_r_curly)
            check_if_missing_r_curly(lexer, args_end_idx+2, func_end_idx,
                    false, NULL);

        *end_idx = func_end_idx;
    }
    else {
        *end_idx = args_end_idx+1;
    }

    ASTNodeList_push_back(&block->nodes, ASTNode_create(
                lexer->token_tbl.elems[f_decl_idx].line_num,
                lexer->token_tbl.elems[f_decl_idx].column_num, ASTType_FUNC,
                func));

    while (vars.size > old_vars_size) {
        ParVarList_pop_back(&vars, ParserVar_free);
    }
    assert(vars.size == old_vars_size);

    m_free(func_name);

}

static u32 parse_ret_stmt(const struct Lexer *lexer, struct BlockNode *block,
        u32 bp, u32 ret_idx, struct FuncDeclNode *parent_func,
        u32 n_stack_frames_deep) {

    struct RetNode *ret_node = safe_malloc(sizeof(*ret_node));
    u32 end_idx;

    if (!parent_func) {
        fprintf(stderr, "return statement outside of a function on line %u\n",
                lexer->token_tbl.elems[ret_idx].line_num);
        Parser_error_occurred = true;
        return skip_to_token_type_alt(ret_idx, lexer->token_tbl,
                TokenType_SEMICOLON);
    }

    *ret_node = RetNode_init();
    ret_node->lvls_of_indir = parent_func->ret_lvls_of_indir;
    ret_node->type = parent_func->ret_type;
    ret_node->n_stack_frames_deep = n_stack_frames_deep;

    if (ret_idx+1 < lexer->token_tbl.size &&
            lexer->token_tbl.elems[ret_idx+1].type == TokenType_SEMICOLON) {
        end_idx = ret_idx+1;
    }
    else if (parent_func->ret_type == PrimType_VOID &&
            parent_func->ret_lvls_of_indir == 0) {
        fprintf(stderr, "cannot return a value in void function '%s'."
                " line %u.\n", parent_func->name,
                lexer->token_tbl.elems[ret_idx].line_num);
        Parser_error_occurred = true;
        end_idx = skip_to_token_type_alt(ret_idx, lexer->token_tbl,
                TokenType_SEMICOLON);
    }
    else {
        enum TokenType stop_types[] = {TokenType_SEMICOLON};
        ret_node->value = SY_shunting_yard(&lexer->token_tbl, ret_idx+1,
                stop_types, sizeof(stop_types)/sizeof(stop_types[0]), &end_idx,
                &vars, bp, false, &typedefs);
        Parser_error_occurred |= SY_error_occurred;
        ret_node->lvls_of_indir = ret_node->value->lvls_of_indir;
        ret_node->type = Expr_type(ret_node->value, &vars);
    }

    if (parent_func->ret_lvls_of_indir >= 1 &&
            parent_func->ret_type != ret_node->type) {
        fprintf(stderr, "'%s' return type and returned type do not match."
                " line %u.\n", parent_func->name,
                lexer->token_tbl.elems[ret_idx].line_num);
        Parser_error_occurred = true;
        end_idx = skip_to_token_type_alt(ret_idx, lexer->token_tbl,
                TokenType_SEMICOLON);
    }

    ASTNodeList_push_back(&block->nodes, ASTNode_create(
                lexer->token_tbl.elems[ret_idx].line_num,
                lexer->token_tbl.elems[ret_idx].column_num, ASTType_RETURN,
                ret_node
                ));

    return end_idx;

}

u32 parse_if_stmt(const struct Lexer *lexer, struct BlockNode *block,
        u32 n_blocks_deep, u32 if_idx, u32 bp, u32 sp,
        struct FuncDeclNode *parent_func) {

    struct IfNode *if_node = NULL;

    u32 r_paren_idx;
    u32 end_idx;

    if (if_idx+1 >= lexer->token_tbl.size ||
            lexer->token_tbl.elems[if_idx+1].type != TokenType_L_PAREN) {
        fprintf(stderr,
                "expected parentheses after the if statement on line %u\n.",
                lexer->token_tbl.elems[if_idx].line_num);
        Parser_error_occurred = true;
        return skip_to_token_type_alt(if_idx, lexer->token_tbl,
                TokenType_SEMICOLON);
    }

    if_node = safe_malloc(sizeof(*if_node));
    *if_node = IfNode_init();

    {
        enum TokenType sy_stop_types[] = {TokenType_R_PAREN};
        if_node->expr = SY_shunting_yard(&lexer->token_tbl, if_idx+2,
                sy_stop_types, sizeof(sy_stop_types)/sizeof(sy_stop_types[0]),
                &r_paren_idx, &vars, bp, false, &typedefs);
        Parser_error_occurred |= SY_error_occurred;
    }

    if (r_paren_idx >= lexer->token_tbl.size) {
        fprintf(stderr,
                "expected a ')' after the condition expression on line %u,"
                " column %u\n", lexer->token_tbl.elems[if_idx+1].line_num,
                lexer->token_tbl.elems[if_idx+1].column_num);
        Parser_error_occurred = true;
        IfNode_free_w_self(if_node);
        return skip_to_token_type_alt(if_idx, lexer->token_tbl,
                TokenType_SEMICOLON);
    }
    else if (r_paren_idx+1 < lexer->token_tbl.size &&
            lexer->token_tbl.elems[r_paren_idx+1].type ==
            TokenType_SEMICOLON) {
        /* idk why tf anyone would make an if statement without a body but here
         * ya go ig */
        ASTNodeList_push_back(&block->nodes, ASTNode_create(
                    lexer->token_tbl.elems[if_idx].line_num,
                    lexer->token_tbl.elems[if_idx].column_num,
                    ASTType_IF_STMT, if_node
                    ));
        return r_paren_idx;
    }
    else if (r_paren_idx+2 >= lexer->token_tbl.size) {
        fprintf(stderr,
                "expected a block after the if statement on line %u.\n",
                lexer->token_tbl.elems[if_idx].line_num);
        Parser_error_occurred = true;
        IfNode_free_w_self(if_node);
        return skip_to_token_type_alt(if_idx, lexer->token_tbl,
                TokenType_SEMICOLON);
    }

    if_node->body_in_block = lexer->token_tbl.elems[r_paren_idx+1].type ==
        TokenType_L_CURLY;
    {
        u32 body_start_idx = r_paren_idx+1+if_node->body_in_block;
        bool missing_r_curly;
        if_node->body = parse(lexer, parent_func,
                if_node->body_in_block ? sp-m_TypeSize_stack_frame_size :
                bp,
                if_node->body_in_block ? sp-m_TypeSize_stack_frame_size :
                sp,
                body_start_idx, &end_idx, n_blocks_deep+if_node->body_in_block,
                &missing_r_curly, if_node->body_in_block,
                !if_node->body_in_block);

        if (!missing_r_curly && if_node->body_in_block)
            check_if_missing_r_curly(lexer, body_start_idx, end_idx, false,
                    NULL);
    }

    if (end_idx+1 < lexer->token_tbl.size &&
            lexer->token_tbl.elems[end_idx+1].type == TokenType_ELSE) {

        u32 else_body_start_idx;
        bool missing_r_curly;
        if_node->else_body_in_block =
            lexer->token_tbl.elems[end_idx+2].type == TokenType_ELSE;
        else_body_start_idx = end_idx+2+if_node->else_body_in_block;
        if_node->else_body = parse(lexer, parent_func,
                if_node->else_body_in_block ? sp-m_TypeSize_stack_frame_size :
                bp,
                if_node->else_body_in_block ? sp-m_TypeSize_stack_frame_size :
                sp,
                else_body_start_idx, &end_idx,
                n_blocks_deep+if_node->else_body_in_block,
                &missing_r_curly, if_node->else_body_in_block,
                !if_node->else_body_in_block);

        if (!missing_r_curly && if_node->else_body_in_block)
            check_if_missing_r_curly(lexer, else_body_start_idx, end_idx,
                    false, NULL);

    }

    ASTNodeList_push_back(&block->nodes, ASTNode_create(
                lexer->token_tbl.elems[if_idx].line_num,
                lexer->token_tbl.elems[if_idx].column_num,
                ASTType_IF_STMT, if_node
                ));

    return end_idx;

}

u32 parse_while_stmt(const struct Lexer *lexer, struct BlockNode *block,
        u32 n_blocks_deep, u32 while_idx, u32 bp, u32 sp,
        struct FuncDeclNode *parent_func) {

    struct WhileNode *while_node = NULL;

    u32 r_paren_idx;
    u32 end_idx;

    if (while_idx+1 >= lexer->token_tbl.size ||
            lexer->token_tbl.elems[while_idx+1].type != TokenType_L_PAREN) {
        fprintf(stderr,
                "expected parentheses after the while statement on line %u\n.",
                lexer->token_tbl.elems[while_idx].line_num);
        Parser_error_occurred = true;
        return skip_to_token_type_alt(while_idx, lexer->token_tbl,
                TokenType_SEMICOLON);
    }

    while_node = safe_malloc(sizeof(*while_node));
    *while_node = WhileNode_init();

    {
        enum TokenType sy_stop_types[] = {TokenType_R_PAREN};
        while_node->expr = SY_shunting_yard(&lexer->token_tbl, while_idx+2,
                sy_stop_types, sizeof(sy_stop_types)/sizeof(sy_stop_types[0]),
                &r_paren_idx, &vars, bp, false, &typedefs);
        Parser_error_occurred |= SY_error_occurred;
    }

    if (r_paren_idx >= lexer->token_tbl.size) {
        fprintf(stderr,
                "expected a ')' after the condition expression on line %u,"
                " column %u\n", lexer->token_tbl.elems[while_idx+1].line_num,
                lexer->token_tbl.elems[while_idx+1].column_num);
        Parser_error_occurred = true;
        WhileNode_free_w_self(while_node);
        return skip_to_token_type_alt(while_idx, lexer->token_tbl,
                TokenType_SEMICOLON);
    }
    else if (r_paren_idx+1 < lexer->token_tbl.size &&
            lexer->token_tbl.elems[r_paren_idx+1].type ==
            TokenType_SEMICOLON) {
        ASTNodeList_push_back(&block->nodes, ASTNode_create(
                    lexer->token_tbl.elems[while_idx].line_num,
                    lexer->token_tbl.elems[while_idx].column_num,
                    ASTType_WHILE_STMT, while_node
                    ));
        return r_paren_idx+1;
    }
    else if (r_paren_idx+2 >= lexer->token_tbl.size) {
        fprintf(stderr,
                "expected a block after the while statement on line %u.\n",
                lexer->token_tbl.elems[while_idx].line_num);
        Parser_error_occurred = true;
        WhileNode_free_w_self(while_node);
        return skip_to_token_type_alt(while_idx, lexer->token_tbl,
                TokenType_SEMICOLON);
    }

    while_node->body_in_block = lexer->token_tbl.elems[r_paren_idx+1].type ==
        TokenType_L_CURLY;
    {
        u32 body_start_idx = r_paren_idx+1+while_node->body_in_block;
        bool missing_r_curly;
        while_node->body = parse(lexer, parent_func,
                while_node->body_in_block ? sp-m_TypeSize_stack_frame_size :
                bp,
                while_node->body_in_block ? sp-m_TypeSize_stack_frame_size :
                sp,
                body_start_idx, &end_idx,
                n_blocks_deep+while_node->body_in_block,
                &missing_r_curly, while_node->body_in_block,
                !while_node->body_in_block);

        if (!missing_r_curly && while_node->body_in_block)
            check_if_missing_r_curly(lexer, body_start_idx, end_idx, false,
                    NULL);
    }

    ASTNodeList_push_back(&block->nodes, ASTNode_create(
                lexer->token_tbl.elems[while_idx].line_num,
                lexer->token_tbl.elems[while_idx].column_num,
                ASTType_WHILE_STMT, while_node
                ));

    return end_idx;

}

u32 parse_for_stmt(const struct Lexer *lexer, struct BlockNode *block,
        u32 n_blocks_deep, u32 for_idx, u32 bp, u32 sp,
        struct FuncDeclNode *parent_func) {

    struct ForNode *for_node = NULL;

    u32 init_end_idx;
    u32 cond_end_idx;
    u32 r_paren_idx;
    u32 end_idx;

    if (for_idx+1 >= lexer->token_tbl.size ||
            lexer->token_tbl.elems[for_idx+1].type != TokenType_L_PAREN) {
        fprintf(stderr,
                "expected parentheses after the for statement on line %u\n.",
                lexer->token_tbl.elems[for_idx].line_num);
        Parser_error_occurred = true;
        return skip_to_token_type_alt(for_idx, lexer->token_tbl,
                TokenType_SEMICOLON);
    }

    for_node = safe_malloc(sizeof(*for_node));
    *for_node = ForNode_init();

    /* get the for loop expressions */

    for_node->init = SY_shunting_yard(&lexer->token_tbl, for_idx+2, NULL, 0,
            &init_end_idx, &vars, bp, false, &typedefs);
    Parser_error_occurred |= SY_error_occurred;
    if (init_end_idx >= lexer->token_tbl.size) {
        fprintf(stderr,
                "expected 3 expressions after the for keyword on line %u.\n",
                lexer->token_tbl.elems[for_idx].line_num);
        Parser_error_occurred = true;
        ForNode_free_w_self(for_node);
        return skip_to_token_type_alt(for_idx, lexer->token_tbl,
                TokenType_SEMICOLON);
    }

    for_node->condition = SY_shunting_yard(&lexer->token_tbl, init_end_idx+1,
            NULL, 0, &cond_end_idx, &vars, bp, false, &typedefs);
    Parser_error_occurred |= SY_error_occurred;
    if (cond_end_idx >= lexer->token_tbl.size) {
        fprintf(stderr,
                "expected 3 expressions after the for keyword on line %u.\n",
                lexer->token_tbl.elems[for_idx].line_num);
        Parser_error_occurred = true;
        ForNode_free_w_self(for_node);
        return skip_to_token_type_alt(for_idx, lexer->token_tbl,
                TokenType_SEMICOLON);
    }

    {
        enum TokenType stop_types[] = {TokenType_R_PAREN};
        for_node->inc = SY_shunting_yard(&lexer->token_tbl, cond_end_idx+1,
                stop_types, sizeof(stop_types)/sizeof(stop_types[0]),
                &r_paren_idx, &vars, bp, false, &typedefs);
        Parser_error_occurred |= SY_error_occurred;
    }
    if (r_paren_idx >= lexer->token_tbl.size) {
        fprintf(stderr,
                "expected a ')' after the 3rd for statement expression on line"
                " %u\n", lexer->token_tbl.elems[for_idx].line_num);
        Parser_error_occurred = true;
        ForNode_free_w_self(for_node);
        return skip_to_token_type_alt(for_idx, lexer->token_tbl,
                TokenType_SEMICOLON);
    }

    if (r_paren_idx+1 < lexer->token_tbl.size &&
            lexer->token_tbl.elems[r_paren_idx+1].type ==
            TokenType_SEMICOLON) {
        ASTNodeList_push_back(&block->nodes, ASTNode_create(
                    lexer->token_tbl.elems[for_idx].line_num,
                    lexer->token_tbl.elems[for_idx].column_num,
                    ASTType_FOR_STMT, for_node
                    ));
        return r_paren_idx+1;
    }
    else if (r_paren_idx+2 >= lexer->token_tbl.size) {
        fprintf(stderr,
                "expected a block after the for statement on line %u.\n",
                lexer->token_tbl.elems[for_idx].line_num);
        Parser_error_occurred = true;
        ForNode_free_w_self(for_node);
        return skip_to_token_type_alt(for_idx, lexer->token_tbl,
                TokenType_SEMICOLON);
    }

    for_node->body_in_block = lexer->token_tbl.elems[r_paren_idx+1].type ==
        TokenType_L_CURLY;
    {
        u32 body_start_idx = r_paren_idx+1+for_node->body_in_block;
        bool missing_r_curly;
        for_node->body = parse(lexer, parent_func,
                for_node->body_in_block ? sp-m_TypeSize_stack_frame_size : bp,
                for_node->body_in_block ? sp-m_TypeSize_stack_frame_size : sp,
                body_start_idx, &end_idx,
                n_blocks_deep+for_node->body_in_block,
                &missing_r_curly, for_node->body_in_block,
                !for_node->body_in_block);

        if (!missing_r_curly && for_node->body_in_block)
            check_if_missing_r_curly(lexer, body_start_idx, end_idx, false,
                    NULL);
    }

    ASTNodeList_push_back(&block->nodes, ASTNode_create(
                lexer->token_tbl.elems[for_idx].line_num,
                lexer->token_tbl.elems[for_idx].column_num,
                ASTType_FOR_STMT, for_node
                ));

    return end_idx;

}

u32 parse_typedef(const struct Lexer *lexer, u32 typedef_idx) {

    u32 conv_type_idx = typedef_idx+1;

    char *type_name = NULL;

    enum PrimitiveType conv_type;
    unsigned conv_lvls_of_indir;
    unsigned type_name_idx = read_type_spec(&lexer->token_tbl, conv_type_idx,
            &conv_type, &conv_lvls_of_indir, &typedefs,
            &Parser_error_occurred); 

    if (lexer->token_tbl.elems[type_name_idx].type != TokenType_IDENT) {
        fprintf(stderr, "expected an identifier at the end of the typedef on"
                " line %u.\n", lexer->token_tbl.elems->line_num);
        Parser_error_occurred = true;
        return skip_to_token_type_alt(typedef_idx, lexer->token_tbl,
                TokenType_SEMICOLON);
    }

    type_name = Token_src(&lexer->token_tbl.elems[type_name_idx]);

    if (Ident_type_spec(type_name, &typedefs) != PrimType_INVALID &&
            (Ident_type_spec(type_name, &typedefs) != conv_type ||
             Ident_type_lvls_of_indir(type_name, &typedefs) !=
             conv_lvls_of_indir)) {
        /* the type already exists and doesn't match the typedef */
        fprintf(stderr, "type '%s' redefined to a different type on line %u,"
                " column %u.\n", type_name,
                lexer->token_tbl.elems[type_name_idx].line_num,
                lexer->token_tbl.elems[type_name_idx].column_num);
        Parser_error_occurred = true;
        m_free(type_name);
    }
    else {
        TypedefList_push_back(&typedefs, Typedef_create(type_name, conv_type,
                    conv_lvls_of_indir));
        type_name = NULL;
    }

    if (lexer->token_tbl.elems[type_name_idx+1].type != TokenType_SEMICOLON) {
        fprintf(stderr, "missing semicolon on line %u.\n",
                lexer->token_tbl.elems[type_name_idx].line_num);
        Parser_error_occurred = true;
    }

    return type_name_idx+1;

}

/*
 * n_instr_to_parse   - if set to 0, parses any nr of instructions.
 */
static struct BlockNode* parse(const struct Lexer *lexer,
        struct FuncDeclNode *parent_func, u32 bp, u32 sp, u32 block_start_idx,
        u32 *end_idx, unsigned n_blocks_deep, bool *missing_r_curly,
        bool detect_missing_curly, u32 n_instr_to_parse) {

    u32 n_instrs_parsed = m_u32_max;  /* wraps around to 0 later */

    u32 old_vars_size = vars.size;
    u32 old_typedefs_size = typedefs.size;

    /* var declarations are only allowed at the top of the scope */
    bool can_decl_vars = true;

    u32 prev_end_idx = block_start_idx-1;
    struct BlockNode *block = safe_malloc(sizeof(*block));
    *block = BlockNode_init();

    if (missing_r_curly)
        *missing_r_curly = false;

    while (prev_end_idx+1 < lexer->token_tbl.size) {

        u32 start_idx = prev_end_idx+1;
        char *token_src = NULL;
        bool declared_var = false;

        ++n_instrs_parsed;
        if (n_instr_to_parse > 0 && n_instrs_parsed == n_instr_to_parse) {
            break;
        }

        token_src = Token_src(&lexer->token_tbl.elems[start_idx]);

        if (lexer->token_tbl.elems[start_idx].type == TokenType_L_CURLY) {
            struct BlockNode *new_block = parse(lexer, parent_func,
                    sp-m_TypeSize_stack_frame_size,
                    sp-m_TypeSize_stack_frame_size, start_idx+1,
                    &prev_end_idx, n_blocks_deep+1, NULL, true, 0);
            ASTNodeList_push_back(&block->nodes, ASTNode_create(
                        lexer->token_tbl.elems[start_idx].line_num,
                        lexer->token_tbl.elems[start_idx].column_num,
                        ASTType_BLOCK, new_block));
            check_if_missing_r_curly(lexer, block_start_idx, prev_end_idx,
                    true, missing_r_curly);
        }
        else if (lexer->token_tbl.elems[start_idx].type == TokenType_R_CURLY) {
            m_free(token_src);
            prev_end_idx = start_idx;
            break;
        }
        else if (strcmp(token_src, "return") == 0) {
            prev_end_idx =
                parse_ret_stmt(lexer, block, bp, start_idx, parent_func,
                        n_blocks_deep);
        }
        else if (Ident_type_spec(token_src, &typedefs) != PrimType_INVALID) {
            unsigned ident_idx = read_type_spec(&lexer->token_tbl, start_idx,
                    NULL, NULL, &typedefs, &Parser_error_occurred);

            /* should probably move this into it's own function at some
             * point */
            if (ident_idx+1 >= lexer->token_tbl.size ||
                    lexer->token_tbl.elems[ident_idx+1].type !=
                    TokenType_L_PAREN) {
                u32 old_sp = sp;
                struct VarDeclNode *var_decl = NULL;

                if (!can_decl_vars) {
                    fprintf(stderr, "mixing declarations and code is a C99"
                            " extension. line %u.\n",
                            lexer->token_tbl.elems[start_idx].line_num);
                    Parser_error_occurred = true;
                }

                var_decl = parse_var_decl(lexer,
                        start_idx, &prev_end_idx, bp, &sp, false, NULL, block);
                ASTNodeList_push_back(&block->nodes,
                        ASTNode_create(
                            lexer->token_tbl.elems[start_idx].line_num,
                            lexer->token_tbl.elems[start_idx].column_num,
                            ASTType_VAR_DECL, var_decl));
                block->var_bytes = block->var_bytes + old_sp-sp;
                declared_var = true;
            }
            else if (lexer->token_tbl.elems[ident_idx+1].type ==
                    TokenType_L_PAREN) {
                parse_func_decl(lexer, block,
                        start_idx, &prev_end_idx, bp);
            }
            else {
                fprintf(stderr,
                        "invalid token '%s' after variable declaration."
                        " line %u, column %u.", token_src,
                        lexer->token_tbl.elems[ident_idx+1].line_num,
                        lexer->token_tbl.elems[ident_idx+1].column_num);
                Parser_error_occurred = true;
            }
        }

        else if (lexer->token_tbl.elems[start_idx].type == TokenType_IF_STMT) {
            prev_end_idx = parse_if_stmt(lexer, block, n_blocks_deep,
                    start_idx, bp, sp, parent_func);
        }

        else if (lexer->token_tbl.elems[start_idx].type ==
                TokenType_WHILE_STMT) {
            prev_end_idx = parse_while_stmt(lexer, block, n_blocks_deep,
                    start_idx, bp, sp, parent_func);
        }

        else if (lexer->token_tbl.elems[start_idx].type ==
                TokenType_FOR_STMT) {
            prev_end_idx = parse_for_stmt(lexer, block, n_blocks_deep,
                    start_idx, bp, sp, parent_func);
        }

        else if (lexer->token_tbl.elems[start_idx].type ==
                TokenType_TYPEDEF) {
            prev_end_idx = parse_typedef(lexer, start_idx);
        }

        else if (lexer->token_tbl.elems[start_idx].type ==
                TokenType_DEBUG_PRINT_RAX) {
            struct DebugPrintRAX *debug_node =
                safe_malloc(sizeof(*debug_node));
            ASTNodeList_push_back(&block->nodes,
                    ASTNode_create(lexer->token_tbl.elems[start_idx].line_num,
                        lexer->token_tbl.elems[start_idx].column_num,
                        ASTType_DEBUG_RAX, debug_node));
            prev_end_idx = start_idx+1;
        }

        else {
            struct Expr *expr = parse_expr(
                    lexer, start_idx, &prev_end_idx, bp);
            struct ExprNode *node = safe_malloc(sizeof(*node));
            node->expr = expr;

            ASTNodeList_push_back(&block->nodes,
                    ASTNode_create(lexer->token_tbl.elems[start_idx].line_num,
                        lexer->token_tbl.elems[start_idx].column_num,
                        ASTType_EXPR, node));
        }

        free(token_src);

        /* check if there is a parent function cuz global variables can be
         * declared anywhere */
        if (!declared_var && parent_func) {
            can_decl_vars = false;
        }

    }

    if (end_idx)
        *end_idx = prev_end_idx;

    block->var_bytes = round_up(block->var_bytes,
            m_TypeSize_stack_min_alignment);

    if (n_blocks_deep == 1 && detect_missing_curly &&
            lexer->token_tbl.elems[prev_end_idx].type != TokenType_R_CURLY) {
        if (missing_r_curly)
            *missing_r_curly = true;
        fprintf(stderr,
                "missing a '{' to go with the '}' on line %u, column %u\n",
                lexer->token_tbl.elems[prev_end_idx].line_num,
                lexer->token_tbl.elems[prev_end_idx].column_num);
    }

    while (vars.size > old_vars_size)
        ParVarList_pop_back(&vars, ParserVar_free);
    assert(vars.size == old_vars_size);

    while (typedefs.size > old_typedefs_size)
        TypedefList_pop_back(&typedefs, Typedef_free);
    assert(typedefs.size == old_typedefs_size);

    return block;

}

struct BlockNode* Parser_parse(const struct Lexer *lexer) {

    u32 bp = 0;
    struct BlockNode *root = NULL;

    vars = ParVarList_init();
    typedefs = TypedefList_init();
    Parser_error_occurred = false;

    root = parse(lexer, NULL, bp, bp, 0, NULL, 0, NULL, true, 0);

    assert(vars.size == 0);
    ParVarList_free(&vars);
    assert(typedefs.size == 0);
    TypedefList_free(&typedefs);
    return root;

}
