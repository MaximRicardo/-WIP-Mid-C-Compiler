#pragma once

#include "../comp_dependent/ints.h"
#include "../utils/vector_impl.h"
#include "../utils/bool.h"

extern bool PreProc_error_occurred;

struct PreProcMacro {

    char *name;
    char *expansion;

};

struct PreProcMacro PreProcMacro_init(void);
struct PreProcMacro PreProcMacro_create(char *name, char *expansion);
void PreProcMacro_free(struct PreProcMacro macro);

struct PreProcMacroList {

    struct PreProcMacro *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(PreProcMacroList, struct PreProcMacro)

struct MacroInstance {

    char *expansion;

    u32 start_idx;

    /* used by #include */
    const char *file_path;

    const struct PreProcMacro *macro;

};

struct MacroInstance MacroInstance_init(void);
struct MacroInstance MacroInstance_create(char *expansion, u32 start_idx,
        const char *file_dir, const struct PreProcMacro *macro);
void MacroInstance_free(struct MacroInstance macro);

struct MacroInstList {

    struct MacroInstance *elems;
    u32 size;
    u32 capacity;

};

m_declare_VectorImpl_funcs(MacroInstList, struct MacroInstance)

/* automatically inits macros and macro_insts */
void PreProc_process(const char *src, struct PreProcMacroList *macros,
        struct MacroInstList *macro_insts, const char *file_path);
