#include "file_io.h"
#include "../comp_dependent/ints.h"
#include "safe_mem.h"
#include <stdio.h>

void copy_file_contents(char *dest, FILE *file)
{
    char c;
    u32 i = 0;

    while ((c = fgetc(file)) != EOF)
        dest[i++] = c;
    dest[i] = '\0';
}

char *copy_file_into_str(FILE *file)
{
    u32 file_size;
    char *str = NULL;

    fseek(file, 0L, SEEK_END);
    file_size = ftell(file);
    rewind(file);

    str = safe_malloc((file_size + 1) * sizeof(*str));
    copy_file_contents(str, file);

    return str;
}
