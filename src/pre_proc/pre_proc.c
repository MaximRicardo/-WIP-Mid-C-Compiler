#include "pre_proc.h"
#include "../comp_dependent/ints.h"
#include "../err_msg.h"
#include "../utils/bool.h"
#include "../utils/safe_mem.h"
#include "../utils/vector_impl.h"
#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

bool PreProc_error_occurred = false;

struct PreProcMacro PreProcMacro_init(void)
{
    struct PreProcMacro macro;
    macro.name = NULL;
    macro.expansion = NULL;
    return macro;
}

struct PreProcMacro PreProcMacro_create(char *name, char *expansion)
{
    struct PreProcMacro macro;
    macro.name = name;
    macro.expansion = expansion;
    return macro;
}

void PreProcMacro_free(struct PreProcMacro macro)
{
    m_free(macro.name);
    m_free(macro.expansion);
}

struct MacroInstance MacroInstance_init(void)
{
    struct MacroInstance macro;
    macro.expansion = NULL;
    macro.start_idx = 0;
    macro.file_path = NULL;
    macro.macro = NULL;
    return macro;
}

struct MacroInstance MacroInstance_create(char *expansion, u32 start_idx,
                                          const char *file_path,
                                          const struct PreProcMacro *macro)
{
    struct MacroInstance macro_inst;
    macro_inst.expansion = expansion;
    macro_inst.start_idx = start_idx;
    macro_inst.file_path = file_path;
    macro_inst.macro = macro;
    return macro_inst;
}

void MacroInstance_free(struct MacroInstance macro)
{
    m_free(macro.expansion);
}

m_define_VectorImpl_funcs(PreProcMacroList, struct PreProcMacro)
    m_define_VectorImpl_funcs(MacroInstList, struct MacroInstance)

        static bool valid_ident_start_char(char c)
{
    return isalpha(c) || c == '_';
}

static bool valid_ident_char(char c)
{
    return isalnum(c) || c == '_';
}

static unsigned get_identifier_len(const char *ident_start)
{
    unsigned i;

    assert(valid_ident_start_char(ident_start[0]));

    for (i = 1; ident_start[i] != '\0'; i++) {
        if (!valid_ident_char(ident_start[i]))
            break;
    }

    return i;
}

static char *sub_str(const char *str, u32 start, u32 len)
{
    char *new_str = safe_malloc((len + 1) * sizeof(*new_str));
    strncpy(new_str, &str[start], len);
    new_str[len] = '\0';

    return new_str;
}

static char *make_str_copy(const char *str)
{
    if (!str) {
        return NULL;
    } else {
        char *new_str = safe_malloc((strlen(str) + 1) * sizeof(*new_str));
        strcpy(new_str, str);
        return new_str;
    }
}

/* returns m_u32_max if the macro couldn't be found */
static u32 find_macro(const struct PreProcMacroList *macros, const char *name)
{
    u32 i;

    for (i = 0; i < macros->size; i++) {
        if (strcmp(name, macros->elems[i].name) == 0)
            return i;
    }

    return m_u32_max;
}

/* returns the resulting string. src is the string to insert */
static void insert_string(char **str, char *src, u32 idx)
{
    char *result =
        safe_malloc((strlen(*str) + strlen(src) + 1) * sizeof(*result));

    strncpy(result, *str, idx);
    result[idx] = '\0';
    strcat(result, src);
    strcat(result, &(*str)[idx]);

    m_free(*str);
    *str = result;
}

/* returns the number of erased characters. start and end are inclusive */
static u32 erase_from_str(char *str, u32 start, u32 end)
{
    u32 i;
    u32 n_erased = end - start + 1;
    u32 str_len = strlen(str);

    for (i = end + 1; i < str_len; i++) {
        str[i - n_erased] = str[i];
    }

    for (i = str_len - n_erased; i <= str_len; i++)
        str[i] = '\0';
    str_len -= n_erased;

    return str_len;
}

/* returns the result of strlen(*str) after expanding the macros */
static u32 expand_macros_in_str(char **str_ptr,
                                const struct PreProcMacroList *macros)
{
    char *str = *str_ptr;
    u32 str_i;

    if (!str)
        return 0;

    for (str_i = 0; str[str_i] != '\0'; str_i++) {
        u32 ident_len;
        char *ident = NULL;
        u32 macro_idx;
        char *macro_expansion = NULL;
        u32 macro_expansion_len;

        if (!valid_ident_start_char(str[str_i]))
            continue;

        ident_len = get_identifier_len(&str[str_i]);
        ident = sub_str(str, str_i, ident_len);

        macro_idx = find_macro(macros, ident);

        if (macro_idx == m_u32_max || !macros->elems[macro_idx].expansion) {
            str_i += ident_len - 1;
            m_free(ident);
            continue;
        }

        macro_expansion = make_str_copy(macros->elems[macro_idx].expansion);
        macro_expansion_len = expand_macros_in_str(&macro_expansion, macros);

        /* erase the macro name from the string */
        erase_from_str(str, str_i, str_i + ident_len - 1);

        /* replace the name with the expansion */
        insert_string(&str, macro_expansion, str_i);

        str_i += macro_expansion_len - 1;

        m_free(macro_expansion);
        m_free(ident);
    }

    *str_ptr = str;
    return strlen(str);
}

/* dir_end points to the first character after the define keyword */
static void read_define_directive(const char *src, u32 dir_end,
                                  unsigned line_num, u32 *end_idx, u32 *n_lines,
                                  const char *file_path,
                                  struct PreProcMacroList *macros)
{
    u32 name_start = dir_end;
    u32 name_len;
    u32 name_end; /* next idx after the name */
    char *name = NULL;

    u32 expansion_end;
    char *expansion = NULL;

    *n_lines = 1;

    while (isspace(src[name_start]) && src[name_start] != '\n')
        ++name_start;

    if (!valid_ident_start_char(src[name_start])) {
        ErrMsg_print(ErrMsg_on, &PreProc_error_occurred, file_path,
                     "expected a macro name on line %u.\n", line_num);
        *end_idx = name_start;
        while (src[*end_idx] != '\n')
            ++*end_idx;
        return;
    }

    name_len = get_identifier_len(&src[name_start]);
    name_end = name_start + name_len;
    name = sub_str(src, name_start, name_len);

    expansion_end = name_end;
    while (src[expansion_end] != '\n')
        ++expansion_end;

    expansion = sub_str(src, name_end, expansion_end - name_end);
    if (expansion[0] == '\0')
        m_free(expansion);

    PreProcMacroList_push_back(macros, PreProcMacro_create(name, expansion));
}

/*
 * end_idx          - *end_idx gets set to the index of the '\n' at the end of
 *                    the directive
 * n_lines          - the number of lines the directive takes up
 */
static void read_preproc_directive(const char *src, u32 hashtag_idx,
                                   unsigned line_num,
                                   struct PreProcMacroList *macros,
                                   u32 *end_idx, unsigned *n_lines,
                                   const char *file_path)
{
    u32 dir_start = hashtag_idx + 1;
    u32 dir_len;
    char *dir = NULL;

    while (src[dir_start] != '\n' && isspace(src[dir_start])) {
        ++dir_start;
    }

    if (!valid_ident_start_char(src[dir_start])) {
        ErrMsg_print(ErrMsg_on, &PreProc_error_occurred, file_path,
                     "expected a pre-processor directive on line %u.\n",
                     line_num);
        while (src[dir_start] != '\n')
            ++dir_start;
        *n_lines = 1;
        *end_idx = dir_start;
    }

    dir_len = get_identifier_len(&src[dir_start]);
    dir = sub_str(src, dir_start, dir_len);

    if (strcmp(dir, "define") == 0) {
        read_define_directive(src, dir_start + dir_len, line_num, end_idx,
                              n_lines, file_path, macros);
    }

    m_free(dir);

    while (src[dir_start] != '\n')
        ++dir_start;
    *end_idx = dir_start;
}

static void create_macro_inst(const struct PreProcMacroList *macros,
                              u32 macro_idx, struct MacroInstList *macro_insts,
                              u32 macro_start_idx, const char *file_path)
{
    char *expansion = NULL;

    assert(macro_idx != m_u32_max);

    expansion = make_str_copy(macros->elems[macro_idx].expansion);
    expand_macros_in_str(&expansion, macros);

    MacroInstList_push_back(
        macro_insts, MacroInstance_create(expansion, macro_start_idx, file_path,
                                          &macros->elems[macro_idx]));
}

static void process(const char *src, struct PreProcMacroList *macros,
                    struct MacroInstList *macro_insts, u32 start_idx,
                    u32 end_idx, const char *file_path)
{
    u32 src_i;
    unsigned line_num = 1;
    unsigned column_num = 1;

    /* only whitespace characters have been found on the current line so far.
     * ignores comments. */
    bool only_whitespace = true;

    for (src_i = start_idx; src_i <= end_idx; src_i++, column_num++) {
        if (src[src_i] == '\n') {
            ++line_num;
            column_num = 0;
            only_whitespace = true;
        }

        else if (src_i + 1 <= end_idx && src[src_i] == '/' &&
                 src[src_i + 1] == '/') {
            ++line_num;
            column_num = 0;

            while (src_i <= end_idx && src[src_i] != '\n') {
                ++src_i;
            }
        }

        else if (src_i + 1 <= end_idx && src[src_i] == '/' &&
                 src[src_i + 1] == '*') {
            while (src_i + 1 <= end_idx &&
                   (src[src_i] != '*' || src[src_i + 1] != '/')) {
                if (src[src_i] == '\n') {
                    ++line_num;
                    column_num = 0;
                }
                ++src_i;
                ++column_num;
            }
            if (src_i + 1 >= end_idx)
                break;
        }

        else if (src[src_i] == '\"') {
            ++src_i;
            while (src_i <= end_idx && src[src_i] != '\"') {
                ++column_num;
                ++src_i;
            }
        } else if (src[src_i] == '\'') {
            ++src_i;
            while (src_i <= end_idx && src[src_i] != '\'') {
                ++column_num;
                ++src_i;
            }
        }

        else if (only_whitespace && src[src_i] == '#') {
            unsigned n_lines;
            read_preproc_directive(src, src_i, line_num, macros, &src_i,
                                   &n_lines, file_path);
            line_num += n_lines;
            column_num = 0;
        }

        else if (valid_ident_start_char(src[src_i])) {
            u32 ident_len = get_identifier_len(&src[src_i]);
            char *ident = sub_str(src, src_i, ident_len);
            u32 macro_idx = find_macro(macros, ident);

            if (macro_idx != m_u32_max) {
                create_macro_inst(macros, macro_idx, macro_insts, src_i,
                                  file_path);
            }

            column_num += ident_len - 1;
            src_i += ident_len - 1;

            m_free(ident);
            only_whitespace = false;
        }

        else if (!isspace(src[src_i])) {
            only_whitespace = false;
        }
    }
}

static void add_builtin_macros(struct PreProcMacroList *macros,
                               const char *file_path)
{
    char *file_macro_expansion =
        safe_malloc((strlen(file_path) + 3) * sizeof(*file_macro_expansion));
    sprintf(file_macro_expansion, "\"%s\"", file_path);

    /* set to the current major version of the compiler */
    PreProcMacroList_push_back(
        macros,
        PreProcMacro_create(make_str_copy("__MID_CC__"), make_str_copy("1")));
    /* set to the current minor version of the compiler */
    PreProcMacroList_push_back(
        macros, PreProcMacro_create(make_str_copy("__MID_CC_MINOR__"),
                                    make_str_copy("0")));
    /* set to the path to the current file */
    PreProcMacroList_push_back(
        macros,
        PreProcMacro_create(make_str_copy("__FILE__"), file_macro_expansion));
}

void PreProc_process(const char *src, struct PreProcMacroList *macros,
                     struct MacroInstList *macro_insts, const char *file_path)
{
    PreProc_error_occurred = false;

    *macros = PreProcMacroList_init();
    *macro_insts = MacroInstList_init();

    add_builtin_macros(macros, file_path);

    process(src, macros, macro_insts, 0, strlen(src), file_path);
}
