#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ast.h"
#include "code_gen.h"
#include "file_io.h"
#include "comp_dependent/ints.h"
#include "safe_mem.h"
#include "lexer.h"
#include "parser.h"
#include "pre_to_post_fix.h"
#include "bin_to_unary.h"
#include "merge_strings.h"

#define m_build_bug_on(condition) \
    ((void)sizeof(char[1 - 2*!!(condition)]))

static char *read_file(char *file_path) {

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

void compile(char *src, const char *src_f_path, FILE *output,
        bool *error_occurred) {

    struct Lexer lexer = Lexer_lex(src, src_f_path);

    *error_occurred = false;

#ifdef m_print_tokens
    u32 i;
    for (i = 0; i < lexer.token_tbl.size; i++) {
        printf("tokens[%u]: line = %d, column = %d, type = %d, value = %d\n",
                i, lexer.token_tbl.tokens[i].line_num,
                lexer.token_tbl.tokens[i].column_num,
                lexer.token_tbl.tokens[i].type,
                lexer.token_tbl.tokens[i].value.int_value);
    }
#endif

    MergeStrings_merge(&lexer.token_tbl);
    BinToUnary_convert(&lexer.token_tbl);
    PreToPostFix_convert(&lexer.token_tbl);

    if (!Lexer_error_occurred) {
        struct BlockNode *ast = Parser_parse(&lexer);

        if (!Parser_error_occurred && output) {
            CodeGen_generate(output, ast);
        }
        else
            *error_occurred = true;

        BlockNode_free_w_self(ast);
    }
    else
        *error_occurred = true;

    Lexer_free(&lexer);

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

    if (argc < 2) {
        fprintf(stderr, "ERROR: No input files.\n");
        return 1;
    }

    src = read_file(argv[1]);
    if (!src) {
        return 1;
    }

    puts(src);

    if (argc >= 3) {
        output = fopen(argv[2], "w");
        if (!output) {
            fprintf(stderr, "can't open file '%s': %s\n", argv[2],
                    strerror(errno));
            return 1;
        }
    }

    compile(src, argv[1], output, &error_occurred);

    m_free(src);
    if (output)
        fclose(output);

    return error_occurred != false;

}
