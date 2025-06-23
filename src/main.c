#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ast.h"
#include "code_gen.h"
#include "comp_args.h"
#include "file_io.h"
#include "comp_dependent/ints.h"
#include "safe_mem.h"
#include "lexer.h"
#include "parser.h"
#include "pre_to_post_fix.h"
#include "bin_to_unary.h"
#include "merge_strings.h"
#include "pre_proc.h"
#include "const_fold.h"
#include "structs.h"

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

static void compile(char *src, FILE *output, bool *error_occurred) {

    struct PreProcMacroList macros;
    struct MacroInstList macro_insts;
    PreProc_process(src, &macros, &macro_insts, CompArgs_args.src_path);
    *error_occurred = false;

    if (!PreProc_error_occurred) {
        struct Lexer lexer = Lexer_lex(src, CompArgs_args.src_path, &macro_insts);

        if (!Lexer_error_occurred) {
            struct TranslUnit tu;

            MergeStrings_merge(&lexer.token_tbl);
            BinToUnary_convert(&lexer.token_tbl);
            PreToPostFix_convert(&lexer.token_tbl);

            tu = Parser_parse(&lexer);

            if (!Parser_error_occurred && output) {
                if (CompArgs_args.optimize) {
                    BlockNode_const_fold(tu.ast);
                }
                CodeGen_generate(output, tu.ast, tu.structs);
            }
            else
                *error_occurred = true;

            while (tu.structs->size > 0) {
                struct Struct back = StructList_back(tu.structs);
                u32 i;

                printf("struct %s {\n", back.name);
                for (i = 0; i < back.members.size; i++) {
                    printf("\t(%d) %s; //lvls of indir %u, offset %u\n",
                            back.members.elems[i].type,
                            back.members.elems[i].name,
                            back.members.elems[i].lvls_of_indir,
                            back.members.elems[i].offset);
                }
                printf("};\n");

                StructList_pop_back(tu.structs, Struct_free);
            }
            StructList_free(tu.structs);
            BlockNode_free_w_self(tu.ast);
        }
        else
            *error_occurred = true;

        Lexer_free(&lexer);
    }
    else
        *error_occurred = true;

    while (macro_insts.size > 0)
        MacroInstList_pop_back(&macro_insts, MacroInstance_free);
    MacroInstList_free(&macro_insts);

    while (macros.size > 0)
        PreProcMacroList_pop_back(&macros, PreProcMacro_free);
    PreProcMacroList_free(&macros);

}

int main(int argc, char *argv[]) {

    char *src = NULL;
    FILE *output = NULL;
    bool error_occurred = false;

    m_build_bug_on(sizeof(i32) != 4);
    m_build_bug_on(sizeof(u32) != 4);
    m_build_bug_on(sizeof(i16) != 2);
    m_build_bug_on(sizeof(u16) != 2);
    m_build_bug_on(sizeof(i8) != 1);
    m_build_bug_on(sizeof(u8) != 1);

    CompArgs_args = CompArgs_get_args(argc, argv);
    if (!CompArgs_args.src_path)
        return 0;

    src = read_file(CompArgs_args.src_path);
    if (!src)
        return 1;
    puts(src);

    if (CompArgs_args.asm_out_path) {
        output = fopen(CompArgs_args.asm_out_path, "w");
        if (!output) {
            fprintf(stderr, "can't open file '%s': %s\n",
                    CompArgs_args.asm_out_path, strerror(errno));
            return 1;
        }
    }

    compile(src, output, &error_occurred);

    m_free(src);
    if (output)
        fclose(output);

    return error_occurred != false;

}
