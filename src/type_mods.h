#pragma once

#include "utils/bool.h"

struct TypeModifiers {

    bool is_static;

};

struct TypeModifiers TypeModifiers_init(void);
struct TypeModifiers TypeModifiers_create(bool is_static);
/* if *logged_errs was already true, it stays true */
struct TypeModifiers TypeModifiers_combine(const struct TypeModifiers *self,
        const struct TypeModifiers *other, bool log_errs, bool *logged_errs);
bool TypeModifiers_equal(const struct TypeModifiers *self,
        const struct TypeModifiers *other);
