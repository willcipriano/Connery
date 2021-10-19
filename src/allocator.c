#include "allocator.h"
#include "cval.h"

#define PREALLOCATE_SLOTS 255
#define PREALLOCATE_ROWS 4

int CUR_OBJ_ID = 0;

typedef struct cval_allocation_array {
    cval** array;
    int size;
    int allocated;
} cval_allocation_array;

typedef struct cval_allocation_index {
    cval_allocation_array** rows;
    int cur;
    int size;
} cval_allocation_index;

cval_allocation_index* INDEX = NULL;

int createObjectId() {
    CUR_OBJ_ID += 1;
    return CUR_OBJ_ID;
}

cval_allocation_array *preallocateArray(int slots) {
    cval_allocation_array* array_struct = malloc(sizeof(cval_allocation_array));
    cval** array = malloc(sizeof(cval *) * slots);
    array_struct->size = slots;
    array_struct->allocated = 0;

    for (int i = 0; i < slots; ++i) {
        cval *nullConst = malloc(sizeof(cval));
        nullConst->type = CVAL_UNALLOCATED;
        nullConst->objId = createObjectId();
        array[i] = nullConst;
    }

    array_struct->array = array;
    return array_struct;
}

cval_allocation_index *preallocateIndex(int rows, int slots) {
    cval_allocation_index* index = malloc(sizeof(cval_allocation_index));
    index->cur = 0;

    for (int i = 0; i < rows; ++i) {
        index->rows[i] = preallocateArray(slots);
    }

    return index;
}

void setup() {
    INDEX = preallocateIndex(PREALLOCATE_ROWS, PREALLOCATE_SLOTS);
}

cval** allocateMany(int total) {
    cval** array = malloc(sizeof(cval *) * total);

    for (int i = 0; i < total; ++i) {
        if (INDEX->rows[INDEX->cur]->allocated == INDEX->rows[INDEX->cur]->size) {
        }
    }


}