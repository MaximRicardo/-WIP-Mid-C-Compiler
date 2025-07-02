#pragma once

/* resizable strings */

#include "../comp_dependent/ints.h"
#include "../utils/macros.h"

#define m_DYNAMIC_STR_start_capacity 128
/* c89 doesn't support vsnprintf, and i don't feel like implementing that
 * manually so i'm just gonna hardcode a maximum buffer value for vsprintf.
 * used by DynamicStr_append_printf */
#define m_DYNAMIC_STR_printf_max_len 1048576

struct DynamicStr {

    /* the raw NULL-terminated string */
    char *str;

    /* size doesn't count the NULL terminator */
    u32 size;
    u32 capacity;

};

struct DynamicStr DynamicStr_init(void);
void DynamicStr_free(struct DynamicStr str);
void DynamicStr_append(struct DynamicStr *self, const char *src);
void DynamicStr_append_dyn(struct DynamicStr *self,
        const struct DynamicStr *other);
/* appends the formatted string to self */
void DynamicStr_append_printf(struct DynamicStr *self, const char *fmt, ...)
    ATTRIBUTE((format (printf, 2, 3)));
void DynamicStr_append_char(struct DynamicStr *self, char c);
/* doesn't do anything if self->size == 0 */
void DynamicStr_pop_back_char(struct DynamicStr *self);
void DynamicStr_shrink_to_fit(struct DynamicStr *self);
