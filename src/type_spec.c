#include "type_spec.h"
#include "ast.h"
#include "err_msg.h"
#include "identifier.h"
#include "safe_mem.h"
#include "token.h"
#include "type_mods.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* returns the index of the token after the last type modifier */
static u32 read_type_modifiers(const struct TokenList *token_tbl, u32 mod_idx,
        struct TypeModifiers *mods) {

    bool is_static = false;

    char *cur_tok_src = Token_src(&token_tbl->elems[mod_idx]);

    while (Ident_modifier_str_to_tok(cur_tok_src) != TokenType_NONE) {

        enum TokenType type = Ident_modifier_str_to_tok(cur_tok_src);

        if (type == TokenType_STATIC)
            is_static = true;
        else
            assert(false);

        m_free(cur_tok_src);
        cur_tok_src = Token_src(&token_tbl->elems[++mod_idx]);

    }

    m_free(cur_tok_src);

    if (mods)
        *mods = TypeModifiers_create(is_static);

    return mod_idx;

}

u32 TypeSpec_read(const struct TokenList *token_tbl, u32 type_spec_idx,
        enum PrimitiveType *type, unsigned *lvls_of_indir,
        struct TypeModifiers *mods,
        const struct TypedefList *typedefs, bool *error_occurred) {

    enum PrimitiveType spec_type;
    unsigned spec_lvls_of_indir;
    struct TypeModifiers spec_mods;
    char *type_name = NULL;
    unsigned n_asterisks = 0;

    type_spec_idx =
        read_type_modifiers(token_tbl, type_spec_idx, mods);

    type_name = Token_src(&token_tbl->elems[type_spec_idx]);

    spec_type = Ident_type_spec(type_name, typedefs);
    spec_lvls_of_indir = Ident_type_lvls_of_indir(type_name, typedefs);
    spec_mods = Ident_type_modifiers(type_name, typedefs);

    while (token_tbl->elems[type_spec_idx+1+n_asterisks].type ==
            TokenType_DEREFERENCE ||
            token_tbl->elems[type_spec_idx+1+n_asterisks].type ==
            TokenType_MUL) {
        ++n_asterisks;
    }

    if (spec_type == PrimType_INVALID) {
        ErrMsg_print(ErrMsg_on, error_occurred,
                token_tbl->elems[type_spec_idx].file_path,
                "unknown type '%s' on line %u, column %u.\n",
                type_name, token_tbl->elems[type_spec_idx].line_num,
                token_tbl->elems[type_spec_idx].column_num);
    }
    else {
        spec_lvls_of_indir += n_asterisks;
    }

    if (type)
        *type = spec_type;
    if (lvls_of_indir)
        *lvls_of_indir = spec_lvls_of_indir;
    if (mods)
        *mods = TypeModifiers_combine(mods, &spec_mods, true, error_occurred);

    m_free(type_name);

    return type_spec_idx+n_asterisks+1;

}
