#include "parser.h"
#include "ast.h"
#include "lexer.h"
#include "shunting_yard.h"
#include <stddef.h>

struct Expr* Parser_parse(const struct Lexer *lexer) {

    return shunting_yard(lexer->token_tbl.elems, 0, lexer->token_tbl.size-1);

}
