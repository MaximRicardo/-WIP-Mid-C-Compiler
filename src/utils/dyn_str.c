#include "dyn_str.h"
#include "safe_mem.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static void grow_to_fit(struct DynamicStr *self, u32 min_capacity) {

    if (self->capacity < min_capacity) {
        self->capacity = min_capacity;
        self->str = safe_realloc(self->str, self->capacity*sizeof(*self->str));
    }

}

/* make sure self->capacity is big enough to fit src appended to self->str */
static void grow_to_fit_new_str(struct DynamicStr *self, const char *src) {

    grow_to_fit(self, self->size+strlen(src)+1);

}

struct DynamicStr DynamicStr_init(void) {

    struct DynamicStr str;
    str.size = 0;
    str.capacity = m_DYNAMIC_STR_start_capacity;
    str.str = safe_malloc(str.capacity*sizeof(*str.str));
    str.str[0] = '\0';
    return str;

}

void DynamicStr_free(struct DynamicStr str) {

    m_free(str.str);

}

void DynamicStr_append(struct DynamicStr *self, const char *src) {

    grow_to_fit_new_str(self, src);
    strcat(self->str, src);
    self->size += strlen(src);

}

void DynamicStr_append_dyn(struct DynamicStr *self,
        const struct DynamicStr *other) {

    grow_to_fit_new_str(self, other->str);
    strcat(self->str, other->str);
    self->size += other->size;

}

void DynamicStr_append_printf(struct DynamicStr *self, const char *fmt, ...) {

    char *new_str = NULL;

    va_list args;
    va_start(args, fmt);

    /* printf and friends return the number of chars printed */
    new_str = safe_malloc((m_DYNAMIC_STR_printf_max_len+1)*sizeof(*new_str));
    vsprintf(new_str, fmt, args);

    DynamicStr_append(self, new_str);

    m_free(new_str);

    va_end(args);

}

void DynamicStr_shrink_to_fit(struct DynamicStr *self) {

    self->str = safe_realloc(self->str, (self->size+1)*sizeof(*self->str));

}

void DynamicStr_append_char(struct DynamicStr *self, char c) {

    grow_to_fit(self, self->size+2);
    self->str[self->size++] = c;
    self->str[self->size] = '\0';

}
