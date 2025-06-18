#include "type_spec.h"
#include "identifier.h"
#include <stdio.h>

u32 read_type_spec(const struct TokenList *token_tbl, u32 type_spec_idx,
        enum PrimitiveType *type, unsigned *lvls_of_indir,
        const struct TypedefList *typedefs, bool *error_occurred) {

    enum PrimitiveType spec_type;
    unsigned spec_lvls_of_indir;
    char *type_name = Token_src(&token_tbl->elems[type_spec_idx]);
    unsigned n_asterisks = 0;

    spec_type = Ident_type_spec(type_name, typedefs);
    spec_lvls_of_indir = Ident_type_lvls_of_indir(type_name, typedefs);

    while (token_tbl->elems[type_spec_idx+1+n_asterisks].type ==
            TokenType_DEREFERENCE ||
            token_tbl->elems[type_spec_idx+1+n_asterisks].type ==
            TokenType_MUL) {
        ++n_asterisks;
    }

    if (spec_type == PrimType_INVALID) {
        fprintf(stderr, "unknown type '%s' on line %u, column %u.\n",
                type_name, token_tbl->elems[type_spec_idx].line_num,
                token_tbl->elems[type_spec_idx].column_num);
        if (error_occurred)
            *error_occurred = true;
    }
    else {
        spec_lvls_of_indir += n_asterisks;
    }

    if (type)
        *type = spec_type;
    if (lvls_of_indir)
        *lvls_of_indir = spec_lvls_of_indir;

    m_free(type_name);

    return type_spec_idx+n_asterisks+1;

}
