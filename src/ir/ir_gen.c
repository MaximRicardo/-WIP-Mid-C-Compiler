#include "ir_gen.h"
#include "func.h"
#include "module.h"
#include <assert.h>
#include <string.h>

static char* make_str_copy(const char *str) {

    u32 len = strlen(str);
    char *new_str = safe_malloc((len+1)*sizeof(*new_str));

    strcpy(new_str, str);

    return new_str;

}

struct IRFunc func_node_gen_ir(const struct FuncDeclNode *func) {

    u32 i;

    const struct ASTNodeList *nodes = &func->body->nodes;

    struct IRFunc ir_func = IRFunc_init();
    ir_func.name = make_str_copy(func->name);

    for (i = 0; i < nodes->size; i++) {

    }

    return ir_func;

}

struct IRModule root_node_gen_ir(const struct BlockNode *root) {

    u32 i;

    struct IRModule module = IRModule_init();

    for (i = 0; i < root->nodes.size; i++) {

        assert(root->nodes.elems[i].type == ASTType_FUNC);

        if (!((struct FuncDeclNode*)root->nodes.elems[i].node_struct)->body)
            continue;

        IRFuncList_push_back(&module.funcs,
                func_node_gen_ir(root->nodes.elems[i].node_struct));

    }

    return module;

}

struct IRModule IRGen_generate(const struct TranslUnit *tu) {

    return root_node_gen_ir(tu->ast);

}
