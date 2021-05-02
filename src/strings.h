#ifndef CONNERY_STRINGS_H
#define CONNERY_STRINGS_H
#include "cval.h"

cval *builtin_string_replace_all(cenv *e, cval *a);
cval *builtin_string_replace_first_char(cenv *e, cval *a);
cval *builtin_find(cenv *e, cval *a);
cval *builtin_split(cenv *e, cval *a);
cval *builtin_concat(cenv *e, cval *a);

#endif //CONNERY_STRINGS_H
