#pragma once

/* Macros to quickly create vector types
 * Requires the vector struct contains elems, size and capacity.
 */

#include <stddef.h>
#include "safe_mem.h"

#define m_define_VectorImpl_funcs(VecStruct, ElemType) \
    m_define_VectorImpl_init(VecStruct) \
    m_define_VectorImpl_free(VecStruct) \
    m_define_VectorImpl_push_back(VecStruct, ElemType) \
    m_define_VectorImpl_pop_back(VecStruct, ElemType) \
    m_define_VectorImpl_back(VecStruct, ElemType) \
    m_define_VectorImpl_back_ptr(VecStruct, ElemType) \
    m_define_VectorImpl_erase(VecStruct, ElemType)

#define m_declare_VectorImpl_funcs(VecStruct, ElemType) \
    struct VecStruct VecStruct##_init(void); \
    void VecStruct##_free(struct VecStruct *self); \
    void VecStruct##_push_back(struct VecStruct *self, ElemType value); \
    void VecStruct##_pop_back(struct VecStruct *self, \
            void free_func(ElemType)); \
    ElemType VecStruct##_back(struct VecStruct *self); \
    ElemType* VecStruct##_back_ptr(struct VecStruct *self); \
    void VecStruct##_erase(struct VecStruct *self, u32 erase_idx, \
            void free_func(ElemType));

#define m_define_VectorImpl_init(VecStruct) \
    struct VecStruct VecStruct##_init(void) { \
        struct VecStruct var; \
        var.elems = NULL; \
        var.size = 0; \
        var.capacity = 0; \
        return var; \
    }

/* DOESN'T FREE ALL THE INDIVIDUAL ELEMENTS! */
#define m_define_VectorImpl_free(VecStruct) \
    void VecStruct##_free(struct VecStruct *self) { \
        self->capacity = 0; \
        m_free(self->elems); \
    }

#define m_define_VectorImpl_push_back(VecStruct, ElemType) \
    void VecStruct##_push_back(struct VecStruct *self, ElemType value) { \
        while (self->size+1 >= self->capacity) { \
            self->capacity = self->capacity >= 2 ? \
                self->capacity*self->capacity : self->capacity+1; \
            self->elems = safe_realloc(self->elems, \
                    self->capacity*sizeof(*self->elems)); \
        } \
        self->elems[self->size++] = value; \
    }

/* optionally, free_func can be NULL, in which case no freeing is done */
#define m_define_VectorImpl_pop_back(VecStruct, ElemType) \
    void VecStruct##_pop_back(struct VecStruct *self, \
            void free_func(ElemType)) { \
        if (free_func) \
            free_func(self->elems[self->size-1]); \
        --self->size; \
    }

#define m_define_VectorImpl_back(VecStruct, ElemType) \
    ElemType VecStruct##_back(struct VecStruct *self) { \
        return self->elems[self->size-1]; \
    }

#define m_define_VectorImpl_back_ptr(VecStruct, ElemType) \
    ElemType* VecStruct##_back_ptr(struct VecStruct *self) { \
        return &self->elems[self->size-1]; \
    }

#define m_define_VectorImpl_erase(VecStruct, ElemType) \
    void VecStruct##_erase(struct VecStruct *self, u32 erase_idx, \
            void free_func(ElemType)) { \
        u32 i; \
        if (free_func) \
            free_func(self->elems[(erase_idx)]); \
        for (i = (erase_idx); self->size > 1 && i < self->size-1; ++i) { \
            self->elems[i] = self->elems[i+1]; \
        } \
        --self->size; \
    }
