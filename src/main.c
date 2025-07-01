#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "code_gen.h"
#include "comp_args.h"
#include "utils/file_io.h"
#include "comp_dependent/ints.h"
#include "utils/safe_mem.h"
#include "front_end/lexer.h"
#include "front_end/parser.h"
#include "front_end/pre_to_post_fix.h"
#include "front_end/bin_to_unary.h"
#include "front_end/merge_strings.h"
#include "pre_proc/pre_proc.h"
#include "transl_unit.h"
#include "compile_via_ir.h"

#define m_gen_ir

#define m_build_bug_on(condition) \
    ((void)sizeof(char[1 - 2*!!(condition)]))

static char *read_file(const char *file_path) {

    char *contents = NULL;
    FILE *file = fopen(file_path, "r");
    if (!file) {
        fprintf(stderr, "Cannot open file \'%s\': %s\n", file_path,
                strerror(errno));
        return NULL;
    }
    contents = copy_file_into_str(file);
    fclose(file);
    return contents;

}

static void free_macros(struct PreProcMacroList *macros,
        struct MacroInstList *macro_insts) {

    while (macro_insts->size > 0)
        MacroInstList_pop_back(macro_insts, MacroInstance_free);
    MacroInstList_free(macro_insts);

    while (macros->size > 0)
        PreProcMacroList_pop_back(macros, PreProcMacro_free);
    PreProcMacroList_free(macros);

}

static void process_tokens(struct Lexer *lexer) {

    MergeStrings_merge(&lexer->token_tbl);
    BinToUnary_convert(&lexer->token_tbl);
    PreToPostFix_convert(&lexer->token_tbl);

}

/* preprocessing */
static void compile(char *src, FILE *output, FILE *mccir_output,
        bool *error_occurred) {

    struct PreProcMacroList macros;
    struct MacroInstList macro_insts;

    struct Lexer lexer;
    struct TranslUnit tu;

    *error_occurred = false;

    PreProc_process(src, &macros, &macro_insts, CompArgs_args.src_path);
    if (PreProc_error_occurred) {
        goto free_macros_ret;
        return;
    }

    lexer = Lexer_lex(src, CompArgs_args.src_path, &macro_insts);
    if (Lexer_error_occurred) {
        goto free_lexer_ret;
        return;
    }

    process_tokens(&lexer);

    tu = Parser_parse(&lexer);
    if (Parser_error_occurred || !output) {
        goto free_tu_ret;
        return;
    }

    if (!CompArgs_args.skip_ir) {
        TranslUnit_compile_via_mccir(&tu, mccir_output, output);
    }
    else {
        /* DEPRECATED! */
        CodeGen_generate(output, tu.ast, tu.structs);
    }

free_tu_ret:
    TranslUnit_free(tu);
free_lexer_ret:
    Lexer_free(&lexer);
free_macros_ret:
    free_macros(&macros, &macro_insts);

}

static FILE* get_asm_out_file(bool *error_occurred) {

    FILE *f = NULL;

    if (CompArgs_args.asm_out_path) {
        f = fopen(CompArgs_args.asm_out_path, "w");
        if (!f) {
            fprintf(stderr, "can't open file '%s': %s\n",
                    CompArgs_args.asm_out_path, strerror(errno));
            *error_occurred = true;
        }
    }

    return f;

}

static FILE* get_mccir_out_file(bool *error_occurred) {

    FILE *f = NULL;

    if (CompArgs_args.mccir_out_path) {
        f = fopen(CompArgs_args.mccir_out_path, "w");
        if (!f) {
            fprintf(stderr, "can't open file '%s': %s\n",
                    CompArgs_args.mccir_out_path, strerror(errno));
            *error_occurred = true;
        }
    }

    return f;

}

int main(int argc, char *argv[]) {

    char *src = NULL;
    FILE *output = NULL;
    FILE *mccir_output = NULL;
    bool error_occurred = false;

    m_build_bug_on(sizeof(i32) != 4);
    m_build_bug_on(sizeof(u32) != 4);
    m_build_bug_on(sizeof(i16) != 2);
    m_build_bug_on(sizeof(u16) != 2);
    m_build_bug_on(sizeof(i8) != 1);
    m_build_bug_on(sizeof(u8) != 1);

    CompArgs_args = CompArgs_get_args(argc, argv);
    if (!CompArgs_args.src_path) {
        goto clean_up_and_ret;
    }

    src = read_file(CompArgs_args.src_path);
    if (!src)
        goto clean_up_and_ret;

    output = get_asm_out_file(&error_occurred);
    if (error_occurred)
        goto clean_up_and_ret;

    mccir_output = get_mccir_out_file(&error_occurred);
    if (error_occurred)
        goto clean_up_and_ret;

    compile(src, output, mccir_output, &error_occurred);

clean_up_and_ret:
    m_free(src);
    if (output)
        fclose(output);
    if (mccir_output)
        fclose(mccir_output);

    return !error_occurred;

}
