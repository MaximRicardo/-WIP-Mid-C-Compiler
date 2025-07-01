#include "type_spec.h"
#include "comp_dependent/ints.h"
#include "err_msg.h"
#include "identifier.h"
#include "prim_type.h"
#include "utils/safe_mem.h"
#include "structs.h"
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

/* returns the index of the next token after the type specifier */
static u32 read_struct_type_spec(const struct TokenList *token_tbl,
        u32 type_spec_idx, u32 *type_idx, unsigned *lvls_of_indir,
        const struct StructList *structs, bool *error_occurred) {

    char *struct_name = NULL;
    u32 struct_name_idx = type_spec_idx+1;
    u32 struct_idx; /* idx in the struct list */

    unsigned local_lvls_of_indir = 0;
    u32 end_idx;

    if (struct_name_idx >= token_tbl->size ||
            token_tbl->elems[struct_name_idx].type != TokenType_IDENT) {
        ErrMsg_print(ErrMsg_on, error_occurred,
                token_tbl->elems[type_spec_idx].file_path,
                "expected a struct name after the 'struct' keyword on line %u,"
                " column %u.\n",
                token_tbl->elems[type_spec_idx].line_num,
                token_tbl->elems[type_spec_idx].column_num);
        return type_spec_idx+1;
    }

    end_idx = struct_name_idx+1;
    while (token_tbl->elems[end_idx].type == TokenType_DEREFERENCE ||
            token_tbl->elems[end_idx].type == TokenType_MUL) {
        ++local_lvls_of_indir;
        ++end_idx;
    }

    if (lvls_of_indir)
        *lvls_of_indir = local_lvls_of_indir;

    struct_name = Token_src(&token_tbl->elems[struct_name_idx]);
    struct_idx = StructList_find_struct(structs, struct_name);

    if (struct_idx == m_u32_max) {
        ErrMsg_print(ErrMsg_on, error_occurred,
                token_tbl->elems[struct_name_idx].file_path,
                "unknown struct '%s' on line %u, column %u.\n",
                struct_name,
                token_tbl->elems[struct_name_idx].line_num,
                token_tbl->elems[struct_name_idx].column_num);

        m_free(struct_name);
        return end_idx;
    }

    if (type_idx)
        *type_idx = struct_idx;

    m_free(struct_name);

    return end_idx;

}

u32 TypeSpec_read(const struct TokenList *token_tbl, u32 type_spec_idx,
        enum PrimitiveType *type, u32 *type_idx, unsigned *lvls_of_indir,
        struct TypeModifiers *mods,
        const struct TypedefList *typedefs, const struct StructList *structs,
        bool *error_occurred) {

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

    if (token_tbl->elems[type_spec_idx].type == TokenType_STRUCT) {
        if (type)
            *type = PrimType_STRUCT;
        return read_struct_type_spec(token_tbl, type_spec_idx, type_idx,
                lvls_of_indir, structs, error_occurred);
    }
    else {
        if (type_idx)
            *type_idx = 0;

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
