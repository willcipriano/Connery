#include "allocator.h"
#include "cval.h"

#define PREALLOCATE_SLOTS 4096
#define PREALLOCATE_ROWS 16
#define ROWS_MAX 4194304
#define MAX_OBJECT_ID 17179869184
#define PRE_CACHE_SIZE 16

typedef struct cval_allocation_array {
    cval **array;
    int size;
    int allocated;
} cval_allocation_array;

typedef struct cval_allocation_index {
    cval_allocation_array **rows;
    int cur;
    int scur;
    int size;
    bool smode;
} cval_allocation_index;

cval_allocation_index *INDEX = NULL;
int CUR_OBJ_ID = 0;
int CUR_PRE_CACHE_POS = 0;
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
    index->size = rows;
    index->rows = malloc(sizeof(cval_allocation_array*) * ROWS_MAX);

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

cval *fetchSmode() {
    if (INDEX->smode) {
        int cur = 1;

        while (INDEX->scur < INDEX->cur) {
            while (cur <= PREALLOCATE_SLOTS) {
                cval* target = INDEX->rows[INDEX->scur]->array[cur];
                if (target->type == CVAL_UNALLOCATED) {
                    return target;
                }
                cur += 1;
            }

        }
            INDEX->scur += 1;
    }
    return NULL;
}


cval **internalCacheFetch(int total) {

    cval **array = malloc(sizeof(cval *) * total);

    for (int i = 0; i < total; ++i) {

        cval* sModeResult = fetchSmode();

        if (sModeResult != NULL) {
            array[i] = sModeResult;
            continue;
        }

        if (sModeResult == NULL) {
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
        INDEX->rows[INDEX->cur]->allocated += 1;
    }
    }

    return array;
}

void allocator_setup() {
    if (!INIT_COMPLETE) {
        OUT_OF_MEMORY_FAULT = cval_fault("You've run out of memory, and thush out of time my young friend.");
        INDEX = preallocateIndex(PREALLOCATE_ROWS, PREALLOCATE_SLOTS);
        preCache = internalCacheFetch(PRE_CACHE_SIZE);
        INIT_COMPLETE = true;
    }
}

cval *allocate() {
    cval* val = NULL;
    if (CUR_PRE_CACHE_POS > PRE_CACHE_SIZE - 1) {
        preCache = internalCacheFetch(PRE_CACHE_SIZE);
        CUR_PRE_CACHE_POS = 0;
    }

    val = preCache[CUR_PRE_CACHE_POS];
    CUR_PRE_CACHE_POS += 1;
    return val;
}

cval **allocateMany(int total) {
    cval** result = malloc(sizeof(cval*) * total);
    int cur = 1;

    while (cur <= total) {
        result[cur] = allocate();
        cur += 1;
    }

    return result;
}

void deallocate(cval* cval) {
    cval->type = CVAL_DELETED;
}

int sweep() {
    int curRow = 1;
    int sweptObj = 0;
    bool rowSet = false;

    while (curRow <= INDEX->size) {
        cval_allocation_array* row = INDEX->rows[curRow - 1];

        int curObject = 1;
        while (curObject <= row->size) {
            cval* object = row->array[curObject - 1];

            if (object->type == CVAL_DELETED) {
                object->num = 0;
                object->fnum = 0;
                object->boolean = false;
                object->type = CVAL_UNALLOCATED;
                row->allocated -= 1;
                sweptObj += 1;

                if (!rowSet) {
                    INDEX->scur = curRow;
                    rowSet = true;
                }
            }

            curObject += 1;
        }
        curRow += 1;
    }

    INDEX->smode = true;
    return sweptObj;
}

cval* mark_and_sweep() {
    int sweptObj = sweep();

    hash_table* ht = hash_table_create(100);
    hash_table_set(ht, "PREALLOCATE_SLOTS", cval_number(PREALLOCATE_SLOTS));
    hash_table_set(ht, "PREALLOCATE_ROWS", cval_number(PREALLOCATE_ROWS));
    hash_table_set(ht, "ROWS_MAX", cval_number(ROWS_MAX));
    hash_table_set(ht, "MAX_OBJECT_ID", cval_number(MAX_OBJECT_ID));
    hash_table_set(ht, "PRE_CACHE_SIZE", cval_number(PRE_CACHE_SIZE));
    hash_table_set(ht, "CURRENT_ROW_ALLOCATED", cval_number(INDEX->rows[INDEX->cur]->allocated));
    hash_table_set(ht, "CURRNET_ROWS_SIZE", cval_number(INDEX->rows[INDEX->cur]->size));
    hash_table_set(ht, "INDEX_CURSOR", cval_number(INDEX->cur));
    hash_table_set(ht, "INDEX_SIZE", cval_number(INDEX->size));
    hash_table_set(ht, "NEXT_OBJECT_ID", cval_number(CUR_OBJ_ID));
    hash_table_set(ht, "SWEPT_OBJECTS", cval_number(sweptObj));
    hash_table_set(ht, "S_MODE", cval_boolean(INDEX->smode));
    hash_table_set(ht, "S_MODE_CURSOR", cval_number(INDEX->scur));

    return cval_dictionary(ht);
}



cval *allocator_status() {
    if (INIT_COMPLETE) {
    hash_table* ht = hash_table_create(100);
    hash_table_set(ht, "PREALLOCATE_SLOTS", cval_number(PREALLOCATE_SLOTS));
    hash_table_set(ht, "PREALLOCATE_ROWS", cval_number(PREALLOCATE_ROWS));
    hash_table_set(ht, "ROWS_MAX", cval_number(ROWS_MAX));
    hash_table_set(ht, "MAX_OBJECT_ID", cval_number(MAX_OBJECT_ID));
    hash_table_set(ht, "PRE_CACHE_SIZE", cval_number(PRE_CACHE_SIZE));
    hash_table_set(ht, "CURRENT_ROW_ALLOCATED", cval_number(INDEX->rows[INDEX->cur]->allocated));
    hash_table_set(ht, "CURRNET_ROWS_SIZE", cval_number(INDEX->rows[INDEX->cur]->size));
    hash_table_set(ht, "INDEX_CURSOR", cval_number(INDEX->cur));
    hash_table_set(ht, "INDEX_SIZE", cval_number(INDEX->size));
    hash_table_set(ht, "NEXT_OBJECT_ID", cval_number(CUR_OBJ_ID));
    hash_table_set(ht, "S_MODE", cval_boolean(INDEX->smode));
    hash_table_set(ht, "S_MODE_CURSOR", cval_number(INDEX->scur));
    return cval_dictionary(ht); }
    return cval_fault("The allocator takesh a wee bit of time to warm up laddy.");
}
