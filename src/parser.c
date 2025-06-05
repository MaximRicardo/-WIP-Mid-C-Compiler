#include "parser.h"
#include "ast.h"
#include "ints.h"
#include "lexer.h"
#include "safe_mem.h"
#include "shunting_yard.h"
#include <stddef.h>
#include <stdio.h>

bool Parser_error_occurred = false;

static struct Expr* parse_expr(const struct Lexer *lexer, u32 start_idx,
        u32 *sy_end_idx) {

    struct Expr *expr = SY_shunting_yard(&lexer->token_tbl, start_idx,
            TokenType_SEMICOLON, sy_end_idx);
    Parser_error_occurred |= SY_error_occurred;

    if (*sy_end_idx == lexer->token_tbl.size) {
        fprintf(stderr, "missing semicolon. line %u\n",
                lexer->token_tbl.elems[start_idx].line_num);
        Parser_error_occurred = true;
    }

    return expr;

}

static struct TUNode *parse(const struct Lexer *lexer) {

    u32 prev_end_idx = m_u32_max; /* causes +1 to wrap around to 0 later */
    struct TUNode *tu = safe_malloc(sizeof(*tu));
    *tu = TUNode_init();

    while (prev_end_idx+1 < lexer->token_tbl.size) {

        u32 start_idx = prev_end_idx+1;
        struct Expr *expr = parse_expr(lexer, start_idx, &prev_end_idx);

        ExprPtrList_push_back(&tu->exprs, expr);

    }

    return tu;

}

struct TUNode* Parser_parse(const struct Lexer *lexer) {

    Parser_error_occurred = false;
    return parse(lexer);

}
