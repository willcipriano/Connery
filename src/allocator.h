#ifndef CONNERY_ALLOCATOR_H
#define CONNERY_ALLOCATOR_H
#include "cval.h"

cval *allocate();
void deallocate(cval* cval);
void allocator_setup();
#endif //CONNERY_ALLOCATOR_H
