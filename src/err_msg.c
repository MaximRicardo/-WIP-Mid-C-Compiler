#include "err_msg.h"
#include "comp_args.h"
#include <stdarg.h>
#include <stdio.h>

bool ErrMsg_on = true;
bool WarnMsg_on = true;

void ErrMsg_print(bool print_err, bool *err_occurred, const char *file_path,
        const char *fmt, ...) {

    va_list args;
    va_start(args, fmt);

    if (print_err) {
        if (err_occurred)
            *err_occurred = true;
        fprintf(stderr, "%s: error: ", file_path);
        vfprintf(stderr, fmt, args);
    }
    else if (err_occurred)
        *err_occurred = false;

    va_end(args);

}

void WarnMsg_print(bool print_warn, bool *err_occurred, const char *file_path,
        const char *fmt, ...) {

    va_list args;
    va_start(args, fmt);

    if (print_warn) {
        if (err_occurred && CompArgs_args.w_error)
            *err_occurred = true;
        fprintf(stderr, "%s: warning: ", file_path);
        vfprintf(stderr, fmt, args);
    }

    va_end(args);

}
