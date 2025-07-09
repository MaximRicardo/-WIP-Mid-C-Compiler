#include "data_type_to_str.h"
#include "../utils/dyn_str.h"

char *IR_data_type_to_str(const struct IRDataType *d_type)
{
    struct DynamicStr str = DynamicStr_init();
    u32 i;

    if (d_type->is_signed)
        DynamicStr_append_printf(&str, "i%u", d_type->width);
    else
        DynamicStr_append_printf(&str, "u%u", d_type->width);

    for (i = 0; i < d_type->lvls_of_indir; i++) {
        DynamicStr_append(&str, "*");
    }

    return str.str;
}
