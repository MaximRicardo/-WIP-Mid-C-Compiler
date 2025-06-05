#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ast.h"
#include "code_gen.h"
#include "file_io.h"
#include "ints.h"
#include "safe_mem.h"
#include "lexer.h"
#include "parser.h"

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

void compile(char *src, FILE *output) {

    struct Lexer lexer = Lexer_lex(src);

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

    if (!Lexer_error_occurred) {
        struct TUNode *tu = Parser_parse(&lexer);

        if (!Parser_error_occurred) {
            CodeGen_generate(output, tu);
        }

        TUNode_free_w_self(tu);
    }

    Lexer_free(&lexer);

}

int main(int argc, char *argv[]) {

    char *src = NULL;
    FILE *output = NULL;

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
    }

    compile(src, output);

    m_free(src);
    if (output)
        fclose(output);

    return 0;

}
