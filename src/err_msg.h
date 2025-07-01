#pragma once

#include "utils/bool.h"
#include "attrib.h"

/* do errors/warnings get emitted? both are true by default */
extern bool ErrMsg_on;
extern bool WarnMsg_on;

/* print_err      - if false, this function does nothing.
 * err_occurred   - gets set to true if print_err is true. makes setting stuff
 *                  like Parser_error_occurred to true whenever an error
 *                  occurred less clunky. can be NULL if you don't need it. */
void ErrMsg_print(bool print_err, bool *err_occurred, const char *file_path,
        const char *fmt, ...) ATTRIBUTE((format (printf, 4, 5)));

/* args do the same as with ErrMsg_print */
void WarnMsg_print(bool print_warn, bool *err_occurred, const char *file_path,
        const char *fmt, ...) ATTRIBUTE((format (printf, 4, 5)));
