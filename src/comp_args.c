#include "comp_args.h"
#include "bool.h"
#include "comp_args_help.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

struct CompArgs CompArgs_init(void) {

    struct CompArgs args;
    args.src_path = NULL;
    args.asm_out_path = NULL;
    args.optimize = false;
    return args;

}

bool err_if_missing_operand(const char *arg, int operand_idx, int argc) {

    if (operand_idx < argc)
        return false;

    fprintf(stderr, "error: '%s' is missing an operand.\n", arg);
    return true;

}

struct CompArgs CompArgs_get_args(int argc, char **argv) {

    struct CompArgs args = CompArgs_init();

    int i;

    for (i = 1; i < argc; i++) {

        if (strcmp(argv[i], "-o") == 0) {
            if (err_if_missing_operand(argv[i], i+1, argc))
                break;
            args.asm_out_path = argv[i+1];
            ++i;
        }

        else if (strcmp(argv[i], "-O") == 0 ||
                strcmp(argv[i], "--optimize") == 0) {
            args.optimize = true;
        }

        else if (strcmp(argv[i], "-h") == 0 ||
                strcmp(argv[i], "--help") == 0) {
            printf("%s", CompArgs_help_str);
        }

        else if (argv[i][0] == '-') {
            fprintf(stderr,
                    "error: command line argument '%s' doesn't exist.\n",
                    argv[i]);
        }

        else {
            args.src_path = argv[i];
        }

    }

    return args;

}
