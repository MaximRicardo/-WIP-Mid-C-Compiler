#include "type_mods.h"
#include "attrib.h"

struct TypeModifiers TypeModifiers_init(void) {

    struct TypeModifiers mods;
    mods.is_static = false;
    return mods;

}

struct TypeModifiers TypeModifiers_create(bool is_static) {

    struct TypeModifiers mods;
    mods.is_static = is_static;
    return mods;

}

struct TypeModifiers TypeModifiers_combine(const struct TypeModifiers *self,
        const struct TypeModifiers *other,
        ATTRIBUTE((unused)) bool log_errs,
        ATTRIBUTE((unused)) bool *logged_errs) {

    struct TypeModifiers new_mods = TypeModifiers_create(
            self->is_static | other->is_static
            );
    return new_mods;

}

bool TypeModifiers_equal(const struct TypeModifiers *self,
        const struct TypeModifiers *other) {

    return self->is_static == other->is_static;

}
