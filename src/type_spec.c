#include "type_spec.h"
#include "err_msg.h"
#include "identifier.h"
#include "prim_type.h"
#include "safe_mem.h"
#include "token.h"
#include "type_mods.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* returns the index of the token after the last type modifier */
static u32 read_type_modifiers(const struct TokenList *token_tbl, u32 mod_idx,
        struct TypeModifiers *mods, bool *is_signed, bool *has_signed_mod,
        bool *error_occurred) {

    bool is_static = false;
    bool signed_mod = false;
    bool unsigned_mod = false;

    char *cur_tok_src = Token_src(&token_tbl->elems[mod_idx]);

    while (Ident_modifier_str_to_tok(cur_tok_src) != TokenType_NONE) {

        enum TokenType type = Ident_modifier_str_to_tok(cur_tok_src);

        if (type == TokenType_UNSIGNED)
            unsigned_mod = true;
        else if (type == TokenType_SIGNED)
            signed_mod = true;
        else if (type == TokenType_STATIC)
            is_static = true;
        else
            assert(false);

        m_free(cur_tok_src);
        cur_tok_src = Token_src(&token_tbl->elems[++mod_idx]);

    }

    m_free(cur_tok_src);

    if (signed_mod && unsigned_mod) {
        ErrMsg_print(ErrMsg_on, error_occurred,
                token_tbl->elems[mod_idx].file_path,
                "cannot mix signed and unsigned modifiers. line %u,"
                " column %u.\n", token_tbl->elems[mod_idx].line_num,
                token_tbl->elems[mod_idx].column_num);
    }

    if (is_signed)
        *is_signed = !unsigned_mod;
    if (has_signed_mod)
        *has_signed_mod = signed_mod;

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
    bool is_signed;
    bool has_signed_mod;
    bool missing_type_spec = false;
    char *type_name = NULL;
    unsigned n_asterisks = 0;

    type_spec_idx =
        read_type_modifiers(token_tbl, type_spec_idx, mods, &is_signed,
                &has_signed_mod, error_occurred);

    type_name = Token_src(&token_tbl->elems[type_spec_idx]);

    spec_type = Ident_type_spec(type_name, typedefs);
    missing_type_spec = spec_type == PrimType_INVALID;
    if (!missing_type_spec) {
        spec_lvls_of_indir = Ident_type_lvls_of_indir(type_name, typedefs);
        spec_mods = Ident_type_modifiers(type_name, typedefs);
    }
    else if (!is_signed || has_signed_mod) {
        /* unsigned and signed on their own default to ints */
        spec_type = PrimType_INT;
        spec_lvls_of_indir = 0;
        spec_mods = TypeModifiers_init();
    }
    else {
        ErrMsg_print(ErrMsg_on, error_occurred,
                token_tbl->elems[type_spec_idx-1].file_path,
                "missing a type specifier on line %u, column %u.\n",
                token_tbl->elems[type_spec_idx-1].line_num,
                token_tbl->elems[type_spec_idx-1].column_num);
        /* defaulting the type to int so everything doesn't crash */
        spec_type = PrimType_INT;
        spec_lvls_of_indir = 0;
        spec_mods = TypeModifiers_init();
    }

    if (!is_signed) {
        if (spec_type == PrimType_VOID) {
            ErrMsg_print(ErrMsg_on, error_occurred,
                    token_tbl->elems[type_spec_idx].file_path,
                    "'void' has no unsigned equivalent. line %u, column %u.\n",
                    token_tbl->elems[type_spec_idx].line_num,
                    token_tbl->elems[type_spec_idx].column_num);
        }
        else
            spec_type = PrimitiveType_make_unsigned(spec_type);
    }

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

    return type_spec_idx-missing_type_spec+n_asterisks+1;

}
