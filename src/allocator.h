#ifndef CONNERY_ALLOCATOR_H
#define CONNERY_ALLOCATOR_H
#include "cval.h"

cval *allocate();
cval **allocateMany(int total);
void deallocate(cval* cval);
void allocator_setup();
cval *allocator_status();
#endif //CONNERY_ALLOCATOR_H
