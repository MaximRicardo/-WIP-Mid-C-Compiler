#include "prim_type.h"
#include "backend_dependent/type_sizes.h"
#include <assert.h>

bool PrimitiveType_signed(enum PrimitiveType type) {

    return type == PrimType_INT || type == PrimType_CHAR;

}

unsigned PrimitiveType_size(enum PrimitiveType type) {

    switch (type) {

    case PrimType_CHAR:
        return m_TypeSize_char;

    case PrimType_INT:
        return m_TypeSize_int;

    case PrimType_VOID:
    case PrimType_INVALID:
        assert(false);

    }

}

enum PrimitiveType PrimitiveType_promote(enum PrimitiveType type) {

    switch (type) {

    case PrimType_CHAR:
    case PrimType_INT:
        return PrimType_INT;

    case PrimType_VOID:
    case PrimType_INVALID:
        assert(false);

    }

}
