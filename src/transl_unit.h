#pragma once

#include "front_end/ast.h"

struct TranslUnit {

    struct BlockNode *ast;
    struct StructList *structs;

};

struct TranslUnit TranslUnit_init(void);
struct TranslUnit TranslUnit_create(struct BlockNode *ast,
        struct StructList *structs);
void TranslUnit_free(struct TranslUnit tu);
