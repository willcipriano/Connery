#include "allocator.h"
#include "cval.h"

#define PREALLOCATE_SLOTS 255
#define PREALLOCATE_ROWS 4
#define ROWS_MAX 32767
#define MAX_OBJECT_ID 8355585
#define PRE_CACHE_SIZE 16

typedef struct cval_allocation_array {
    cval **array;
    int size;
    int allocated;
} cval_allocation_array;

typedef struct cval_allocation_index {
    cval_allocation_array **rows;
    int cur;
    int size;
    bool oom;
} cval_allocation_index;

cval_allocation_index *INDEX = NULL;
int CUR_OBJ_ID = 0;
int CUR_PRE_CACHE_POS = NULL;
bool INIT_COMPLETE = false;

cval *OUT_OF_MEMORY_FAULT = NULL;
cval **preCache = NULL;

int createObjectId() {
    CUR_OBJ_ID += 1;

    if (CUR_OBJ_ID <= MAX_OBJECT_ID) {
        return CUR_OBJ_ID;
    }

    return -1;
}

cval_allocation_array *preallocateArray(int slots) {
    cval_allocation_array *array_struct = malloc(sizeof(cval_allocation_array));
    cval **array = malloc(sizeof(cval *) * slots);
    array_struct->size = slots;
    array_struct->allocated = 0;

    for (int i = 0; i < slots; ++i) {
        int objId = createObjectId();
        if (objId != -1) {
            cval *nullConst = malloc(sizeof(cval));
            nullConst->type = CVAL_UNALLOCATED;
            nullConst->objId = objId;
            array[i] = nullConst;
        } else {
            return NULL;
        }
    }
    array_struct->array = array;
    return array_struct;
}

cval_allocation_index *preallocateIndex(int rows, int slots) {
    cval_allocation_index *index = malloc(sizeof(cval_allocation_index));
    index->cur = 0;
    index->rows = malloc(sizeof(cval_allocation_array*) * ROWS_MAX);

    if (OUT_OF_MEMORY_FAULT == NULL) {
        OUT_OF_MEMORY_FAULT = cval_fault("You've run out of memory, and thush out of time my young friend.");
    }

    for (int i = 0; i < rows; ++i) {
        cval_allocation_array *array = preallocateArray(slots);
        if (array != NULL) {
            index->rows[i] = array;
        }
    }

    for (int i = rows; i < ROWS_MAX; ++i) {
        index->rows[i] = NULL;
    }

    return index;
}

void setup();

cval **allocateMany(int total) {

    cval **array = malloc(sizeof(cval *) * total);

    for (int i = 0; i < total; ++i) {
        // allocate more slots when out of memory, returning out of memory fault
        // on failure.
        if (INDEX->rows[INDEX->cur]->allocated == INDEX->rows[INDEX->cur]->size) {
            if (INDEX->cur == INDEX->size) {
                if (INDEX->cur < ROWS_MAX) {
                    INDEX->cur += 1;
                    INDEX->size += 1;
                    INDEX->rows[INDEX->cur] = preallocateArray(PREALLOCATE_SLOTS);
                } else {
                    array[i] = OUT_OF_MEMORY_FAULT;
                    return array;
                }
            } else {
                INDEX->cur += 1;
            }
        }
        array[i] = INDEX->rows[INDEX->cur]->array[INDEX->rows[INDEX->cur]->allocated];
    }

    return array;
}

void allocator_setup() {
    INDEX = preallocateIndex(PREALLOCATE_ROWS, PREALLOCATE_SLOTS);
    preCache = allocateMany(PRE_CACHE_SIZE);
    CUR_PRE_CACHE_POS = 0;
    INIT_COMPLETE = true;
}

cval *allocate() {
    cval* val = NULL;
    if (CUR_PRE_CACHE_POS > PRE_CACHE_SIZE - 1) {
        preCache = allocateMany(PRE_CACHE_SIZE);
        CUR_PRE_CACHE_POS = 0;
    }

    val = preCache[CUR_PRE_CACHE_POS];
    CUR_PRE_CACHE_POS += 1;
    return val;
}

