#ifndef CONNERY_ALLOCATOR_H
#define CONNERY_ALLOCATOR_H
#include "cval.h"

cval *allocate();
cval **allocateMany(int total);
void deallocate(cval* cval);
void allocator_setup();
cval *allocator_status();
cval* mark_and_sweep(cenv* env);
long get_row_by_id(long id);
long get_index_by_row_and_id(long id, long row);
cval *object_by_id(long id);
void index_shutdown();
bool ALLOCATOR_MEMORY_PRESSURE;
#endif //CONNERY_ALLOCATOR_H
