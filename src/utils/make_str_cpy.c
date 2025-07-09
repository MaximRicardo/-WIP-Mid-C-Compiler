#include "make_str_cpy.h"
#include "safe_mem.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

char *make_str_copy(const char *str)
{
    if (!str) {
        return NULL;
    } else {
        char *new_str = safe_malloc((strlen(str) + 1) * sizeof(*new_str));
        strcpy(new_str, str);
        return new_str;
    }
}
