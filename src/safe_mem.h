#pragma once

#include <stdlib.h>

/* Frees the pointer and sets it to NULL */
#define m_free(ptr) \
    do { \
        free(ptr); \
        ptr = NULL; \
    } while (0)

void* safe_malloc(size_t size);

void* safe_calloc(size_t num, size_t size);

void* safe_realloc(void* ptr, size_t size);
