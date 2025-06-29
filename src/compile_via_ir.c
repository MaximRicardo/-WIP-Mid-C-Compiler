#include "compile_via_ir.h"
#include "comp_args.h"
#include "ir/ir_gen.h"
#include "ir/ir_to_str.h"
#include "ir/opt_alloca.h"
#include "ir/opt_unused_vregs.h"
#include "ir/opt_copy_prop.h"
#include "ir/ssa_to_tac.h"
#include "ir/x86/code_gen.h"
#include "ir/x86/alloca_to_esp.h"
#include "ir/x86/virt_to_phys.h"

static void output_ir_contents(const struct IRModule *ir,
        const char *header, FILE *output) {

    char *str = IRToStr_gen(ir);

    fprintf(output, ";--------------------------------------\n");
    fprintf(output, ";%s\n", header);
    fprintf(output, ";--------------------------------------\n");
    fputs(str, output);
    fflush(output);

    m_free(str);

}

void TranslUnit_compile_via_mccir(struct TranslUnit *tu, FILE *mccir_output,
        FILE *asm_output) {

    struct IRModule ir = IRGen_generate(tu);
    char *asm_str = NULL;

    if (mccir_output)
        output_ir_contents(&ir, "Pre-Optimizations", mccir_output);

    if (CompArgs_args.optimize) {
        IROpt_alloca(&ir);
        IROpt_unused_vregs(&ir);
        IROpt_copy_prop(&ir);
    }

    if (mccir_output)
        output_ir_contents(&ir, "Post-Optimizations", mccir_output);

    IR_ssa_to_tac(&ir);

    X86_alloca_to_esp(&ir);
    X86_virt_to_phys(&ir);

    if (mccir_output)
        output_ir_contents(&ir, "Final State", mccir_output);

    asm_str = gen_x86_from_ir(&ir);
    fputs(asm_str, asm_output);

    m_free(asm_str);
    IRModule_free(ir);

}
