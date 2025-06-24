#include "comp_args.h"
#include "bool.h"
#include "comp_args_help.h"
#include "version.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

struct CompArgs CompArgs_args;

struct CompArgs CompArgs_init(void) {

    struct CompArgs args;
    memset(&args, 0, sizeof(args));
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

        else if (strcmp(argv[i], "-mccir") == 0) {
            if (err_if_missing_operand(argv[i], i+1, argc))
                break;
            args.mccir_out_path = argv[i+1];
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

        else if (strcmp(argv[i], "-v") == 0 ||
                strcmp(argv[i], "--version") == 0) {
            printf("MCC %u.%u\n", m_MCC_major_version, m_MCC_minor_version);
        }

        else if (strcmp(argv[i], "-Werror") == 0) {
            args.w_error = true;
        }
        else if (strcmp(argv[i], "--pedantic") == 0) {
            args.pedantic = true;
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
