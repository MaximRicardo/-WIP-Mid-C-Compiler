#include "parser.h"
#include "ast.h"
#include "comp_dependent/ints.h"
#include "lexer.h"
#include "safe_mem.h"
#include "shunting_yard.h"
#include "identifier.h"
#include "token.h"
#include "backend_dependent/type_sizes.h"
#include "macros.h"
#include "parser_var.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

bool Parser_error_occurred = false;
struct ParVarList vars;

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

/* same as skip_to_token_type but if the token couldn't be found tokens.size-1,
 * or the index of the last token, is returned instead */
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
            sy_end_idx, &vars, bp);
    Parser_error_occurred |= SY_error_occurred;

    if (*sy_end_idx == lexer->token_tbl.size) {
        fprintf(stderr, "missing semicolon. line %u.\n",
                lexer->token_tbl.elems[start_idx].line_num);
        Parser_error_occurred = true;
    }

    return expr;

}

static struct Expr* var_decl_value(const struct Lexer *lexer, u32 ident_idx,
        u32 *semicolon_idx, u32 bp) {

    struct Expr *expr = NULL;

    if (ident_idx+2 >= lexer->token_tbl.size ||
            lexer->token_tbl.elems[ident_idx+1].type == TokenType_SEMICOLON) {
        *semicolon_idx = ident_idx+1;
        return NULL;
    }

    if (lexer->token_tbl.elems[ident_idx+1].type != TokenType_EQUAL) {
        fprintf(stderr, "missing an equals sign. line %u.\n",
                lexer->token_tbl.elems[ident_idx].line_num);
        Parser_error_occurred = true;
        *semicolon_idx = ident_idx;
        while (lexer->token_tbl.elems[*semicolon_idx].type !=
                TokenType_SEMICOLON)
            ++*semicolon_idx;
        return NULL;
    }

    expr = SY_shunting_yard(&lexer->token_tbl, ident_idx+2, NULL, 0,
            semicolon_idx, &vars, bp);
    Parser_error_occurred |= SY_error_occurred;

    return expr;

}

/*
 * n_func_param_bytes   - how many bytes of parameters/arguments does the func
 *                        have thus far? can be NULL if is_func_param is false.
 * sp                   - can be NULL if is_func_param is true.
 */
static struct VarDeclNode* parse_var_decl(const struct Lexer *lexer,
        enum PrimitiveType var_type, u32 v_decl_idx, u32 *end_idx, u32 bp,
        u32 *sp, bool is_func_param, unsigned *n_func_param_bytes) {

    /* rounded up to the closest multiple of
     * m_TypeSize_stack_var_min_alignment */
    unsigned var_size;

    struct VarDeclNode *var_decl = NULL;
    struct Expr *expr = NULL;
    struct Declarator decl;

    unsigned n_asterisks = 0;
    unsigned ident_idx = v_decl_idx+1;
    while (v_decl_idx+1+n_asterisks < lexer->token_tbl.size &&
            lexer->token_tbl.elems[v_decl_idx+1+n_asterisks].type ==
                TokenType_MUL) {
        ++ident_idx;
        ++n_asterisks;
    }

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
    else if (var_type == PrimType_VOID) {
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
    
    var_size = round_up(PrimitiveType_size(var_type, n_asterisks),
            m_TypeSize_stack_var_min_alignment);
    var_decl = safe_malloc(sizeof(*var_decl));
    expr = is_func_param ? NULL :
        var_decl_value(lexer, ident_idx, end_idx, bp);
    decl = Declarator_create(expr,
            Token_src(&lexer->token_tbl.elems[ident_idx]), n_asterisks, 0);

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

    *var_decl = VarDeclNode_init();
    var_decl->type = var_type;
    DeclList_push_back(&var_decl->decls, decl);

    ParVarList_push_back(&vars, ParserVar_create(
                lexer->token_tbl.elems[v_decl_idx].line_num,
                lexer->token_tbl.elems[v_decl_idx].column_num,
                Token_src(&lexer->token_tbl.elems[ident_idx]), n_asterisks,
                var_type, decl.bp_offset+bp, NULL, false, false));
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
    not_matching |= vars.elems[prev_func_decl_var_idx].type != func->ret_type;
    not_matching |= vars.elems[prev_func_decl_var_idx].args->size !=
        func->args.size;
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
    return not_matching;

}

static bool is_unnamed_void_var(const struct Lexer *lexer, u32 type_spec_idx) {

    char *type_spec_src = Token_src(&lexer->token_tbl.elems[type_spec_idx]);

    bool is_true = (Ident_type_spec(type_spec_src) == PrimType_VOID &&
            (type_spec_idx+1 >= lexer->token_tbl.size ||
                (lexer->token_tbl.elems[type_spec_idx+1].type !=
                 TokenType_IDENT &&
                 lexer->token_tbl.elems[type_spec_idx+1].type !=
                 TokenType_MUL)));

    m_free(type_spec_src);
    return is_true;

}

/* returns the right parenthesis of after the function arguments */
static u32 parse_func_args(const struct Lexer *lexer, u32 arg_decl_start_idx,
        struct VarDeclPtrList *args, bool *void_args, u32 bp) {

    u32 arg_decl_idx = arg_decl_start_idx;
    u32 arg_decl_end_idx = arg_decl_idx;
    u32 n_func_param_bytes = 0;

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

        char *type_spec_src = Token_src(&lexer->token_tbl.elems[arg_decl_idx]);
        struct VarDeclNode *arg;

        if (Ident_type_spec(type_spec_src) == PrimType_INVALID) {
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

        arg = parse_var_decl(lexer, Ident_type_spec(type_spec_src),
                arg_decl_idx, &arg_decl_end_idx, bp, NULL, true,
                &n_func_param_bytes);

        if (arg)
            VarDeclPtrList_push_back(args, arg);

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
        enum PrimitiveType func_type, u32 f_decl_idx, u32 *end_idx, u32 bp) {

    struct FuncDeclNode *func = safe_malloc(sizeof(*func));
    struct VarDeclPtrList args = VarDeclPtrList_init();
    bool void_args = false;
    u32 old_vars_size = vars.size;
    u32 args_end_idx;
    char *func_name = NULL;
    u32 prev_func_decl_var_idx;

    u32 func_n_asterisks = 0;
    u32 f_ident_idx = f_decl_idx+1;
    while (lexer->token_tbl.elems[f_decl_idx+1+func_n_asterisks].type ==
            TokenType_MUL) {
        ++f_ident_idx;
        ++func_n_asterisks;
    }

    func_name = Token_src(&lexer->token_tbl.elems[f_ident_idx]);

    prev_func_decl_var_idx = ParVarList_find_var(&vars, func_name);

    if (prev_func_decl_var_idx == m_u32_max) { 
        /* has no earlier declaration */
        ParVarList_push_back(&vars, ParserVar_create(
                    lexer->token_tbl.elems[f_decl_idx].line_num,
                    lexer->token_tbl.elems[f_decl_idx].column_num,
                    Token_src(&lexer->token_tbl.elems[f_ident_idx]),
                    func_n_asterisks, func_type, 0, &func->args, void_args,
                    false));
        ++old_vars_size;
    }

    args_end_idx = parse_func_args(lexer, f_ident_idx+2, &args, &void_args, bp);
    vars.elems[old_vars_size-1].void_args = void_args;

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

    *func = FuncDeclNode_create(args, void_args, func_n_asterisks, func_type,
                NULL, Token_src(&lexer->token_tbl.elems[f_ident_idx]));

    if (prev_func_decl_var_idx != m_u32_max && func_prototypes_match(lexer,
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
    else if (parent_func->ret_type == PrimType_VOID) {
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
                &vars, bp);
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
                &r_paren_idx, &vars, bp);
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
                lexer->token_tbl.elems[if_idx+1].line_num);
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
                body_start_idx, &end_idx, n_blocks_deep+1,
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
                else_body_start_idx, &end_idx, n_blocks_deep+1,
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

/*
 * n_instr_to_parse   - if set to 0, parses any nr of instructions.
 */
static struct BlockNode* parse(const struct Lexer *lexer,
        struct FuncDeclNode *parent_func, u32 bp, u32 sp, u32 block_start_idx,
        u32 *end_idx, unsigned n_blocks_deep, bool *missing_r_curly,
        bool detect_missing_curly, u32 n_instr_to_parse) {

    u32 n_instrs_parsed = m_u32_max;  /* wraps around to 0 later */

    u32 old_vars_size = vars.size;

    u32 prev_end_idx = block_start_idx-1;
    struct BlockNode *block = safe_malloc(sizeof(*block));
    *block = BlockNode_init();

    if (missing_r_curly)
        *missing_r_curly = false;

    while (prev_end_idx+1 < lexer->token_tbl.size) {

        u32 start_idx = prev_end_idx+1;
        char *token_src = NULL;

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
        else if (Ident_type_spec(token_src) != PrimType_INVALID) {
            unsigned ident_idx = start_idx+1;
            while (ident_idx < lexer->token_tbl.size &&
                    lexer->token_tbl.elems[ident_idx].type == TokenType_MUL) {
                ++ident_idx;
            }
            if (ident_idx+1 >= lexer->token_tbl.size ||
                    lexer->token_tbl.elems[ident_idx+1].type !=
                    TokenType_L_PAREN) {
                u32 old_sp = sp;
                struct VarDeclNode *var_decl = parse_var_decl(lexer,
                        Ident_type_spec(token_src), start_idx, &prev_end_idx,
                        bp, &sp, false, NULL);
                ASTNodeList_push_back(&block->nodes,
                        ASTNode_create(
                            lexer->token_tbl.elems[start_idx].line_num,
                            lexer->token_tbl.elems[start_idx].column_num,
                            ASTType_VAR_DECL, var_decl));
                block->var_bytes = block->var_bytes + old_sp-sp;
                assert(sp % m_TypeSize_stack_var_min_alignment == 0);
                assert(
                    block->var_bytes % m_TypeSize_stack_var_min_alignment == 0
                    );
            }
            else if (lexer->token_tbl.elems[ident_idx+1].type ==
                    TokenType_L_PAREN) {
                parse_func_decl(lexer, block, Ident_type_spec(token_src),
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

    }

    if (end_idx)
        *end_idx = prev_end_idx;

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

    return block;

}

struct BlockNode* Parser_parse(const struct Lexer *lexer) {

    u32 bp = 0;
    struct BlockNode *root = NULL;

    vars = ParVarList_init();
    Parser_error_occurred = false;

    root = parse(lexer, NULL, bp, bp, 0, NULL, 0, NULL, true, 0);

    assert(vars.size == 0);
    ParVarList_free(&vars);
    return root;

}
