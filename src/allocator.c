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

long get_row_by_id(long id) {
    long row = 1;

    while ((row * PREALLOCATE_SLOTS) < id) {
        row += 1;
    }

    return row;
}

long get_index_by_row_and_id(long id, long row) {
    return (id - ((row - 1) * PREALLOCATE_SLOTS));
}

cval_allocation_index *INDEX = NULL;
int CUR_OBJ_ID = 0;
int CUR_PRE_CACHE_POS = 0;
bool INIT_COMPLETE = false;
bool ALLOCATOR_MEMORY_PRESSURE = false;

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
    cval **array = calloc(sizeof(cval*), slots);
    array_struct->size = slots;
    array_struct->allocated = 0;

    for (int i = 0; i < slots; ++i) {
        int objId = createObjectId();
        if (objId != -1) {
            cval *nullConst = malloc(sizeof(cval));
            nullConst->type = CVAL_UNALLOCATED;
            nullConst->objId = objId;
            nullConst->cell = NULL;
            nullConst->formals = NULL;
            nullConst->body = NULL;
            array[i] = nullConst;
        } else {
            return NULL;
        }
    }
    array_struct->array = array;
    return array_struct;
}

cval_allocation_index *preallocateIndex(int rows, int slots) {
    cval_allocation_index *index = malloc(sizeof(cval_allocation_index*));
    index->cur = 0;
    index->size = rows;
    index->rows = calloc(sizeof(cval_allocation_array*), ROWS_MAX);

    for (int i = 0; i <= rows; ++i) {
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
                if (target != NULL) {
                if (target->type == CVAL_REALLOCATED) {
                    target->type = CVAL_UNALLOCATED;
                    return target;
                }}
                cur += 1;
            }
            INDEX->scur += 1;
        }
    }
    return NULL;
}


cval **internalCacheFetch(int total) {
    cval **array = calloc(sizeof(cval*), total);

    for (int i = 0; i < total; ++i) {

        cval* sModeResult = fetchSmode();
        if (sModeResult != NULL) {
            array[i] = sModeResult;
            continue;
        }

        if (INDEX->rows[INDEX->cur]->allocated == INDEX->rows[INDEX->cur]->size) {
            if (INDEX->cur == INDEX->size - 1) {
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
        array[i]->type = CVAL_UNALLOCATED;
        INDEX->rows[INDEX->cur]->allocated += 1;
    }

    return array;
}

void allocator_setup() {
    if (!INIT_COMPLETE) {
        OUT_OF_MEMORY_FAULT = malloc(sizeof(cval));
        OUT_OF_MEMORY_FAULT->type = CVAL_FAULT;
        OUT_OF_MEMORY_FAULT->err = "You've run out of memory, and thush out of time my young friend.";
        INDEX = preallocateIndex(PREALLOCATE_ROWS, PREALLOCATE_SLOTS);
        preCache = internalCacheFetch(PRE_CACHE_SIZE);
        INIT_COMPLETE = true;
    }
}

cval *allocate() {
    cval* val = NULL;

    if (CUR_PRE_CACHE_POS > PRE_CACHE_SIZE - 1) {
        if (preCache != NULL) {
            free(preCache);
        }
        preCache = internalCacheFetch(PRE_CACHE_SIZE);
        CUR_PRE_CACHE_POS = 0;
    }

    val = preCache[CUR_PRE_CACHE_POS];
    CUR_PRE_CACHE_POS += 1;
    return val;
}

void deallocate(cval* cval) {
    cval->deleted = true;
}

long markValue(cval* val);

int markDictionary(cval* dictionary) {
    cval** dictContent = hash_table_dump_values(dictionary->ht);
    int items = dictionary->ht->items;
    int totalMarked = items;
    int cur = 0;

    while (cur + 1 <= items) {
        markValue(dictContent[cur]);
        cur += 1;
    }

    free(dictContent);
    return totalMarked;
}

long markEnv(cenv* env);

long markValue(cval* val) {
    long marked = 0;
    if (val != NULL) {
        val->mark = true;
        marked += 1;

        for (int i = 0; i < val->count; i++) {
            marked += markValue(val->cell[i]);
        }

        marked += markValue(val->formals);
        marked += markValue(val->body);

        if (val->type == CVAL_DICTIONARY) {
            marked += markDictionary(val);
        }
    }

    return marked;
}

long markEnv(cenv* env) {
    cenv* curEnv = env;
    long totalMarked = 0;

    while (curEnv != NULL) {
     cval** envContents = hash_table_dump_values(curEnv->ht);
     long size = curEnv->ht->items;
     int cur = 0;

    while (cur < size - 1) {
        totalMarked += markValue(envContents[cur]);
        cur += 1;
    }

    curEnv = curEnv->par;
    free(envContents);
    }

    return totalMarked;
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

            if (object->deleted || (!object->mark && object->type != CVAL_UNALLOCATED && object->type != CVAL_REALLOCATED)) {

                switch (object->type) {
                    case CVAL_NUMBER:
                        object->num = 0;
                        break;

                    case CVAL_BOOLEAN:
                        object->boolean = false;
                        break;

                    case CVAL_FLOAT:
                        object->fnum = 0.0;
                        break;

                    case CVAL_SYMBOL:
                        free(object->sym);
                        break;

                    case CVAL_FAULT:
                        free(object->err);
                        break;

                    case CVAL_STRING:
                        free(object->str);
                        break;
                }
                object->type = CVAL_REALLOCATED;
                object->deleted = false;
                row->allocated -= 1;
                sweptObj += 1;

                if (!rowSet) {
                    INDEX->scur = curRow - 1;
                    rowSet = true;
                }
            }
            object->mark = false;
            curObject += 1;
        }
        curRow += 1;
    }

    INDEX->smode = true;
    return sweptObj;
}

cval *allocatorStatus(long sweptObj, long markedObj){
    hash_table* ht = hash_table_create(100);
    hash_table_set(ht, "PREALLOCATE_SLOTS", cval_number(PREALLOCATE_SLOTS));
    hash_table_set(ht, "PREALLOCATE_ROWS", cval_number(PREALLOCATE_ROWS));
    hash_table_set(ht, "INDEX_ROWS_MAX", cval_number(ROWS_MAX));
    hash_table_set(ht, "INDEX_MAX_OBJECT_ID", cval_number(MAX_OBJECT_ID));
    hash_table_set(ht, "INDEX_PRE_CACHE_SIZE", cval_number(PRE_CACHE_SIZE));
    hash_table_set(ht, "ROW_ALLOCATED", cval_number(INDEX->rows[INDEX->cur]->allocated));
    hash_table_set(ht, "ROW_SIZE", cval_number(INDEX->rows[INDEX->cur]->size));
    hash_table_set(ht, "INDEX_CURSOR", cval_number(INDEX->cur));
    hash_table_set(ht, "INDEX_SIZE", cval_number(INDEX->size));
    hash_table_set(ht, "INDEX_NEXT_OBJECT_ID", cval_number(CUR_OBJ_ID));

    if (sweptObj != -1) {
    hash_table_set(ht, "SWEPT_OBJECTS", cval_number(sweptObj)); }

    if (markedObj != -1) {
        hash_table_set(ht, "MARKED_OBJECTS", cval_number(markedObj)); }

    hash_table_set(ht, "S_MODE", cval_boolean(INDEX->smode));
    hash_table_set(ht, "S_MODE_CURSOR", cval_number(INDEX->scur));
    return cval_dictionary(ht);
}


cval* mark_and_sweep(cenv* env) {
    long sweptObj = 0;
    long markedObj = 3;

    if (INIT_COMPLETE) {
        markedObj = markEnv(env);

        NULL_CVAL_CONSTANT->mark = true;
        TRUE_CVAL_CONSTANT->mark = true;
        FALSE_CVAL_CONSTANT->mark = true;

        sweptObj = sweep();
    }

    preCache = internalCacheFetch(PRE_CACHE_SIZE);
    return allocatorStatus(sweptObj, markedObj);
}



cval *allocator_status() {
    if (INIT_COMPLETE) {
        return allocatorStatus(-1, -1);
     }
    return cval_fault("The allocator takesh a wee bit of time to warm up laddy.");
}

cval *object_by_id(long id) {
    long row = get_row_by_id(id);
    return INDEX->rows[row]->array[get_index_by_row_and_id(id, row)];
}

