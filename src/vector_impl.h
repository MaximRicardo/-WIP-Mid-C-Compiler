#pragma once

/* Macros to quickly create vector types
 * Requires the vector struct contains elems, size and capacity.
 */

#include <stddef.h>
#include "safe_mem.h"

#define m_define_VectorImpl_init(VecStruct) \
    struct VecStruct VecStruct##_init(void) { \
        struct VecStruct var; \
        var.elems = NULL; \
        var.size = 0; \
        var.capacity = 0; \
        return var; \
    }

#define m_define_VectorImpl_free(VecStruct) \
    void VecStruct##_free(struct VecStruct *self) { \
        self->capacity = 0; \
        m_free(self->elems); \
    }

#define m_define_VectorImpl_push_back(VecStruct, ElemsType) \
    void VecStruct##_push_back(struct VecStruct *self, ElemsType value) { \
        while (self->size+1 >= self->capacity) { \
            self->capacity = self->capacity >= 2 ? \
                self->capacity*self->capacity : self->capacity+1; \
            self->elems = safe_realloc(self->elems, \
                    self->capacity*sizeof(*self->elems)); \
        } \
        self->elems[self->size++] = value; \
    }

#define m_define_VectorImpl_pop_back(VecStruct) \
    void VecStruct##_pop_back(struct VecStruct *self) { \
        --self->size; \
    }
