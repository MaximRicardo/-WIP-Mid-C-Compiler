#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "code_gen.h"
#include "ir/x86/alloca_to_esp.h"
#include "ir/x86/code_gen.h"
#include "comp_args.h"
#include "file_io.h"
#include "comp_dependent/ints.h"
#include "ir/module.h"
#include "safe_mem.h"
#include "lexer.h"
#include "parser.h"
#include "pre_to_post_fix.h"
#include "bin_to_unary.h"
#include "merge_strings.h"
#include "pre_proc.h"
#include "transl_unit.h"
#include "ir/ir_gen.h"
#include "ir/ir_to_str.h"
#include "ir/opt_alloca.h"
#include "ir/ssa_to_tac.h"
#include "ir/x86/virt_to_phys.h"

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
        free_macros(&macros, &macro_insts);
        return;
    }

    lexer = Lexer_lex(src, CompArgs_args.src_path, &macro_insts);
    if (Lexer_error_occurred) {
        Lexer_free(&lexer);
        free_macros(&macros, &macro_insts);
        return;
    }

    MergeStrings_merge(&lexer.token_tbl);
    BinToUnary_convert(&lexer.token_tbl);
    PreToPostFix_convert(&lexer.token_tbl);

    tu = Parser_parse(&lexer);
    if (Parser_error_occurred || !output) {
        TranslUnit_free(tu);
        Lexer_free(&lexer);
        free_macros(&macros, &macro_insts);
        return;
    }

    if (!CompArgs_args.skip_ir) {
        struct IRModule ir_tu = IRGen_generate(&tu);
        char *asm_output = NULL;

        if (CompArgs_args.optimize) {
            IROpt_alloca(&ir_tu);
        }
        IR_ssa_to_tac(&ir_tu);

        X86_alloca_to_esp(&ir_tu);
        X86_virt_to_phys(&ir_tu);

        if (mccir_output) {
            char *ir_output_str = IRToStr_gen(&ir_tu);
            fputs(ir_output_str, mccir_output);
            fflush(mccir_output);
            m_free(ir_output_str);
        }

        /*
        asm_output = gen_x86_from_ir(&ir_tu, &tu);
        fputs(asm_output, output);
        */

        m_free(asm_output);
        IRModule_free(ir_tu);
    }
    else {
        CodeGen_generate(output, tu.ast, tu.structs);
    }

    TranslUnit_free(tu);
    Lexer_free(&lexer);
    free_macros(&macros, &macro_insts);

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
    if (!src) {
        goto clean_up_and_ret;
    }

    if (CompArgs_args.asm_out_path) {
        output = fopen(CompArgs_args.asm_out_path, "w");
        if (!output) {
            fprintf(stderr, "can't open file '%s': %s\n",
                    CompArgs_args.asm_out_path, strerror(errno));
            error_occurred = true;
            goto clean_up_and_ret;
        }
    }

    if (CompArgs_args.mccir_out_path) {
        mccir_output = fopen(CompArgs_args.mccir_out_path, "w");
        if (!mccir_output) {
            fprintf(stderr, "can't open file '%s': %s\n",
                    CompArgs_args.mccir_out_path, strerror(errno));
            error_occurred = true;
            goto clean_up_and_ret;
        }
    }

    compile(src, output, mccir_output, &error_occurred);

clean_up_and_ret:
    m_free(src);
    if (output)
        fclose(output);
    if (mccir_output)
        fclose(mccir_output);

    return !error_occurred;

}
