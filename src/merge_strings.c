#include "merge_strings.h"
#include "safe_mem.h"
#include "token.h"
#include <string.h>

void MergeStrings_merge(struct TokenList *token_tbl) {

    u32 i;

    for (i = 0; token_tbl->size >= 2 && i < token_tbl->size-1; i++) {

        if (token_tbl->elems[i].type != TokenType_STR_LIT ||
                token_tbl->elems[i+1].type != TokenType_STR_LIT)
            continue;

        token_tbl->elems[i].value.string = safe_realloc(
                token_tbl->elems[i].value.string,
                (strlen(token_tbl->elems[i].value.string)+
                     strlen(token_tbl->elems[i+1].value.string)+1) *
                sizeof(*token_tbl->elems[i].value.string));
        strcat(token_tbl->elems[i].value.string,
                token_tbl->elems[i+1].value.string);

        TokenList_erase(token_tbl, i+1, Token_free);

        --i;

    }

}
