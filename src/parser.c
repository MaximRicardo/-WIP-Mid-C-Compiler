#include "parser.h"
#include "ast.h"
#include "comp_dependent/ints.h"
#include "lexer.h"
#include "safe_mem.h"
#include "shunting_yard.h"
#include "identifier.h"
#include "token.h"
#include "backend_dependent/type_sizes.h"
#include "vector_impl.h"
#include "macros.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

bool Parser_error_occurred = false;
struct ParVarList vars;

static struct BlockNode* parse(const struct Lexer *lexer, u32 bp,
        u32 start_idx, u32 *end_idx, unsigned n_blocks_deep,
        bool *missing_r_curly);

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

struct ParserVar ParserVar_init(void) {

    struct ParserVar var;
    var.line_num = 0;
    var.column_num = 0;
    var.name = NULL;
    var.type = PrimType_INVALID;
    var.stack_pos = 0;
    var.args = NULL;
    var.has_been_defined = false;
    return var;

}

struct ParserVar ParserVar_create(unsigned line_num, unsigned column_num,
        char *name, enum PrimitiveType type, u32 stack_pos,
        struct VarDeclPtrList *args, bool has_been_defined) {

    struct ParserVar var;
    var.line_num = line_num;
    var.column_num = column_num;
    var.name = name;
    var.type = type;
    var.stack_pos = stack_pos;
    var.args = args;
    var.has_been_defined = has_been_defined;
    return var;

}

void ParserVar_free(struct ParserVar var) {

    m_free(var.name);

}

u32 ParVarList_find_var(const struct ParVarList *self, char *name) {

    unsigned i;
    for (i = 0; i < self->size; i++) {
        if (strcmp(self->elems[i].name, name) == 0)
            return i;
    }

    return m_u32_max;

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

static struct Expr* var_decl_value(const struct Lexer *lexer, u32 v_decl_idx,
        u32 *semicolon_idx, u32 bp) {

    struct Expr *expr = NULL;

    if (v_decl_idx+3 >= lexer->token_tbl.size ||
            lexer->token_tbl.elems[v_decl_idx+2].type == TokenType_SEMICOLON) {
        *semicolon_idx = v_decl_idx+2;
        return NULL;
    }

    if (lexer->token_tbl.elems[v_decl_idx+2].type != TokenType_EQUAL) {
        fprintf(stderr, "missing an equals sign. line %u.\n",
                lexer->token_tbl.elems[v_decl_idx].line_num);
        Parser_error_occurred = true;
        *semicolon_idx = v_decl_idx+1;
        while (lexer->token_tbl.elems[*semicolon_idx].type !=
                TokenType_SEMICOLON)
            ++*semicolon_idx;
        return NULL;
    }

    expr = SY_shunting_yard(&lexer->token_tbl, v_decl_idx+3, NULL, 0,
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

    unsigned var_size = PrimitiveType_size(var_type);
    struct VarDeclNode *var_decl = safe_malloc(sizeof(*var_decl));
    struct Expr *expr = is_func_param ? NULL :
        var_decl_value(lexer, v_decl_idx, end_idx, bp);
    struct Declarator decl = Declarator_create(expr,
            Token_src(&lexer->token_tbl.elems[v_decl_idx+1]), 0);
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
        *end_idx = v_decl_idx+2;
    }

    *var_decl = VarDeclNode_init();
    var_decl->type = var_type;
    DeclList_push_back(&var_decl->decls, decl);

    ParVarList_push_back(&vars, ParserVar_create(
                lexer->token_tbl.elems[v_decl_idx].line_num,
                lexer->token_tbl.elems[v_decl_idx].column_num,
                Token_src(&lexer->token_tbl.elems[v_decl_idx+1]), var_type,
                decl.bp_offset+bp, NULL, false));
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
        u32 f_decl_tok_idx) {

    char *func_name = Token_src(&lexer->token_tbl.elems[f_decl_tok_idx+1]);
    u32 i;
    bool not_matching = false;
    not_matching |= vars.elems[prev_func_decl_var_idx].type != func->ret_type;
    not_matching |= vars.elems[prev_func_decl_var_idx].args->size !=
        func->args.size;
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

/* returns the right parenthesis of after the function arguments */
static u32 parse_func_args(const struct Lexer *lexer, u32 arg_decl_start_idx,
        struct VarDeclPtrList *args, u32 bp) {

    u32 arg_decl_idx = arg_decl_start_idx;
    u32 arg_decl_end_idx = arg_decl_idx;
    u32 n_func_param_bytes = 0;

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
    u32 old_vars_size = vars.size;
    u32 args_end_idx;
    char *func_name = Token_src(&lexer->token_tbl.elems[f_decl_idx+1]);

    u32 prev_func_decl_var_idx;
    prev_func_decl_var_idx = ParVarList_find_var(&vars, func_name);

    if (prev_func_decl_var_idx == m_u32_max) { 
        /* has no earlier prototype */
        ParVarList_push_back(&vars, ParserVar_create(
                    lexer->token_tbl.elems[f_decl_idx].line_num,
                    lexer->token_tbl.elems[f_decl_idx].column_num,
                    Token_src(&lexer->token_tbl.elems[f_decl_idx+1]),
                    func_type, 0, &func->args, false));
        ++old_vars_size;
    }

    args_end_idx = parse_func_args(lexer, f_decl_idx+3, &args, bp);

    *func = FuncDeclNode_create(args, func_type, NULL,
                    Token_src(&lexer->token_tbl.elems[f_decl_idx+1]));

    if (prev_func_decl_var_idx != m_u32_max && func_prototypes_match(lexer,
                prev_func_decl_var_idx, func, f_decl_idx)) {
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

        func->body = parse(lexer, bp, args_end_idx+2, &func_end_idx, 1,
                &missing_r_curly);
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

static struct BlockNode* parse(const struct Lexer *lexer, u32 bp,
        u32 block_start_idx, u32 *end_idx, unsigned n_blocks_deep,
        bool *missing_r_curly) {

    /* used for bp offsets */
    u32 sp = bp;

    u32 old_vars_size = vars.size;

    u32 prev_end_idx = block_start_idx-1;
    struct BlockNode *block = safe_malloc(sizeof(*block));
    *block = BlockNode_init();

    if (missing_r_curly)
        *missing_r_curly = false;

    while (prev_end_idx+1 < lexer->token_tbl.size) {

        u32 start_idx = prev_end_idx+1;
        char *token_src = Token_src(&lexer->token_tbl.elems[start_idx]);

        if (lexer->token_tbl.elems[start_idx].type == TokenType_L_CURLY) {
            struct BlockNode *new_block = parse(lexer,
                    sp-m_TypeSize_stack_frame_size, start_idx+1,
                    &prev_end_idx, n_blocks_deep+1, NULL);
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
        else if (Ident_type_spec(token_src) != PrimType_INVALID) {
            if (start_idx+2 >= lexer->token_tbl.size ||
                    lexer->token_tbl.elems[start_idx+2].type !=
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
                block->var_bytes = round_up(block->var_bytes + old_sp-sp, 8);
                sp = round_down(sp, 8);
            }
            else if (lexer->token_tbl.elems[start_idx+2].type ==
                    TokenType_L_PAREN) {
                parse_func_decl(lexer, block, Ident_type_spec(token_src),
                        start_idx, &prev_end_idx, bp);
            }
            else {
                fprintf(stderr,
                        "invalid token '%s' after variable declaration."
                        " line %u, column %u.", token_src,
                        lexer->token_tbl.elems[start_idx+2].line_num,
                        lexer->token_tbl.elems[start_idx+2].column_num);
                Parser_error_occurred = true;
            }
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

    if (n_blocks_deep == 1 &&
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

    root = parse(lexer, bp, 0, NULL, 0, NULL);

    assert(vars.size == 0);
    ParVarList_free(&vars);
    return root;

}

m_define_VectorImpl_funcs(ParVarList, struct ParserVar)
