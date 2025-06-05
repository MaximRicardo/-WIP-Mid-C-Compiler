#include "parser.h"
#include "ast.h"
#include "ints.h"
#include "lexer.h"
#include "safe_mem.h"
#include "shunting_yard.h"
#include "identifier.h"
#include "token.h"
#include "type_sizes.h"
#include "vector_impl.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

bool Parser_error_occurred = false;
struct ParVarList vars;

struct ParserVar ParserVar_init(void) {

    struct ParserVar var;
    var.name = NULL;
    var.type = PrimType_INVALID;
    var.stack_pos = 0;
    return var;

}

struct ParserVar Variable_create(char *name, enum PrimitiveType type,
        u32 stack_pos) {

    struct ParserVar var;
    var.name = name;
    var.type = type;
    var.stack_pos = stack_pos;
    return var;

}

void Variable_free(struct ParserVar var) {

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

static struct Expr* parse_expr(const struct Lexer *lexer, u32 start_idx,
        u32 *sy_end_idx, u32 bp) {

    struct Expr *expr = SY_shunting_yard(&lexer->token_tbl, start_idx,
            TokenType_SEMICOLON, sy_end_idx, &vars, bp);
    Parser_error_occurred |= SY_error_occurred;

    if (*sy_end_idx == lexer->token_tbl.size) {
        fprintf(stderr, "missing semicolon. line %u\n",
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
        fprintf(stderr, "missing an equals sign. line %u\n",
                lexer->token_tbl.elems[v_decl_idx].line_num);
        *semicolon_idx = v_decl_idx+2;
        while (lexer->token_tbl.elems[v_decl_idx].type != TokenType_SEMICOLON)
            ++v_decl_idx;
        return NULL;
    }

    expr = SY_shunting_yard(&lexer->token_tbl, v_decl_idx+3,
            TokenType_SEMICOLON, semicolon_idx, &vars, bp);
    Parser_error_occurred |= SY_error_occurred;

    return expr;

}

static void parse_var_decl(const struct Lexer *lexer, struct TUNode *tu,
        u32 v_decl_idx, u32 *end_idx, u32 bp, u32 *sp) {

    struct VarDeclNode *var_decl = safe_malloc(sizeof(*var_decl));
    struct Expr *expr =
        var_decl_value(lexer, v_decl_idx, end_idx, bp);
    struct Declarator decl = Declarator_create(expr,
            Token_src(&lexer->token_tbl.elems[v_decl_idx+1]));

    *var_decl = VarDeclNode_init();
    DeclList_push_back(&var_decl->decls, decl);

    ParVarList_push_back(&vars, Variable_create(
                Token_src(&lexer->token_tbl.elems[v_decl_idx+1]), PrimType_INT,
                *sp-8));
    ASTNodeList_push_back(&tu->nodes,
            ASTNode_create(ASTType_VAR_DECL, var_decl));
    sp -= m_TypeSize_var_stack_size;

    if (lexer->token_tbl.elems[*end_idx].type != TokenType_SEMICOLON) {
        fprintf(stderr, "missing semicolon. line %u\n",
                lexer->token_tbl.elems[v_decl_idx].line_num);
        Parser_error_occurred = true;
    }

}

static struct TUNode* parse(const struct Lexer *lexer) {

    /* these are used for bp offsets */
    u32 bp = 0;
    u32 sp = 0;

    u32 prev_end_idx = m_u32_max; /* causes +1 to wrap around to 0 later */
    struct TUNode *tu = safe_malloc(sizeof(*tu));
    *tu = TUNode_init();

    while (prev_end_idx+1 < lexer->token_tbl.size) {

        u32 start_idx = prev_end_idx+1;
        char *token_src = Token_src(&lexer->token_tbl.elems[start_idx]);

        if (Ident_type_spec(token_src) != PrimType_INVALID) {
            parse_var_decl(lexer, tu, start_idx, &prev_end_idx, bp, &sp);
        }
        else {
            struct Expr *expr = parse_expr(
                    lexer, start_idx, &prev_end_idx, bp);
            struct ExprNode *node = safe_malloc(sizeof(*node));
            node->expr = expr;

            ASTNodeList_push_back(&tu->nodes,
                    ASTNode_create(ASTType_EXPR, node));
        }

        free(token_src);

    }

    while (vars.size > 0)
        ParVarList_pop_back(&vars, Variable_free);
    ParVarList_free(&vars);

    return tu;

}

struct TUNode* Parser_parse(const struct Lexer *lexer) {

    Parser_error_occurred = false;
    return parse(lexer);

}

m_define_VectorImpl_funcs(ParVarList, struct ParserVar)
